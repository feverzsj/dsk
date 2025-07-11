// #define SIMDJSON_VERBOSE_LOGGING 1
#ifndef NDEBUG
    #define DSK_USE_STD_UNORDERED
#endif
#include <dsk/util/debug.hpp>
#include <dsk/util/deque.hpp>
#include <dsk/util/vector.hpp>
#include <dsk/util/string.hpp>
#include <dsk/util/endian.hpp>
#include <dsk/util/num_cast.hpp>
#include <dsk/util/from_str.hpp>
#include <dsk/util/small_vector.hpp>
#include <dsk/util/perf_counter.hpp>
#include <dsk/jser/dsl.hpp>
#include <dsk/jser/json/simdjson.hpp>
#include <dsk/sqlite3/conn.hpp>
#include <dsk/compr/auto_decompressor.hpp>
#include <dsk/asio/file.hpp>
#include <dsk/res_queue.hpp>
#include <dsk/sync_wait.hpp>
#include <dsk/start_on.hpp>
#include <dsk/until.hpp>
#include <dsk/task.hpp>
#include <backward.hpp>
#include <cmath>
#include <fstream>
#include "classify.hpp"


backward::SignalHandling signalHandling;


using namespace dsk;

// https://doc.wikimedia.org/Wikibase/master/php/docs_topics_json.html#json_datavalues_time
struct wiki_date
{
    int16_t year  = 0; // < 0 for BCE, > 0 for AD, 0 is invalid.
    uint8_t month = 0; // 0 means unspecified, only year is effective
    uint8_t day   = 0; // 0 means unspecified, only year is effective

    constexpr auto operator<=>(wiki_date const&) const noexcept = default;

    constexpr bool valid() const noexcept
    {
        return year != 0 && month <= 12 && day <= 31;
    }

    explicit constexpr operator bool() const noexcept
    {
        return valid();
    }

    constexpr int32_t to_int32() const noexcept
    {
        DSK_ASSERT(valid());
        if(year >= 0) return year * 10000 + month * 100 + day;
        else          return year * 10000 - (12 - month) * 100 - (31 - day);
                                            // make the BCE dates monotonically increasing within same year
    }

    constexpr errc from_int32(int32_t v) noexcept
    {        
        int32_t t = std::abs(v);

        year  =  t / 10000;
        month = (t -= year * 10000) / 100;
        day   =  t - month * 100;

        if(year < 0)
        {
            year  = -year;
            month = 12 - month;
            day   = 31 - day;
        }

        if(! valid()) return errc::invalid_input;
        else          return {};
    }

    template<class T = string>
    constexpr T to_str() const
    {
        DSK_ASSERT(valid());
        return cat_as_str<T>((year > 0 ? "+" : ""), year, '-', month, '-', day, "T00:00:00Z");
    }

    constexpr error_code from_str(_byte_str_ auto const& in) noexcept
    {
        wiki_date d;

        auto s = str_view<char>(in);

        if(s.starts_with('+')) s.remove_prefix(1);

        DSK_E_TRY(size_t n, dsk::from_str(s, d.year));
        if(d.year == 0) return errc::invalid_input;

        if(s.size() == n) return {};
        if(s[n] != '-') return errc::invalid_input;
        s.remove_prefix(n + 1);

        DSK_E_TRY(n, dsk::from_str(s, d.month));
        if(d.month > 12) return errc::invalid_input;

        if(s.size() == n || s[n] != '-') return errc::invalid_input;
        s.remove_prefix(n + 1);

        DSK_E_TRY(n, dsk::from_str(s, d.day));
        if(d.day > 31) return errc::invalid_input;

        if(s.size() > n && s[n] != 'T') return errc::invalid_input;

        *this = d;
        return {};
    }

    static constexpr double tolong(double v) noexcept
    {
        return v >= 0 ? floor(v) : ceil(v);
    }

    // https://core2.gsfc.nasa.gov/time/julian.html
    template<class T = int>
    constexpr expected<int, errc> to_jdn() const noexcept
    {
        if(! valid()) return errc::invalid_input;

        double y = ((year < 0) ? (year + 1) : (year));
        double m = ((month > 2) ? (month + 1) : (--y, month + 13));

        double n = tolong(floor(365.25 * y) + floor(30.6001 * m) + day + 1720995);

        if(day + 31 * (month + 12 * year) >= 14 + 31 * (10 + 12*1582))
        {
            double a = tolong(0.01 * y);
            n += 2 - a + tolong(0.25 * a);
        }

        wiki_date d;
        DSK_E_TRY_ONLY(d.from_jdn(n));

        if(*this != d) return errc::invalid_input;
        else           return num_cast<T>(n);
    }

    static constexpr expected<wiki_date, errc> from_jdn(double n) noexcept
    {
        //n = tolong(n);

        if(n > 2299160)
        {
            double a = tolong(((n - 1867216) - 0.25) / 36524.25);
            n += 1 + a - tolong(0.25 * a);
        }

        double b = n + 1524;
        double c = tolong(6680 + ((b - 2439870) - 122.1) / 365.25);
        double d = tolong(365 * c + 0.25 * c);
        double e = tolong((b - d) / 30.6001);

        double jy = c - 4715;
        double jm = e - 1;
        double jd = b - d - tolong(30.6001 * e);

        if(jm > 12) jm -= 12;
        if(jm > 2 ) --jy;
        if(jy <= 0) --jy;

        if(jy < SHRT_MIN || jy > SHRT_MAX ||  jm < 0 || jm > 12 || jd < 0 || jd > 31)
        {
            return errc::invalid_input;
        }

        return wiki_date(jy, jm, jd);
    }
};

inline constexpr double coord_scale = 1e7;

struct wiki_coord
{
    int32_t lon, lat;
};

struct wiki_item
{
    unsigned id;// "Qnnnnn"
    string title; // labels->en->value
    small_vector<uint32_t, 16> classes; // claims->P31->mainsnak->datavalue->value->id  Qid of class    
    optional<uint32_t> subClassOf; // claims->P279->mainsnak->datavalue->value->id
    bool sport;                    // claims->P641
    optional<uint32_t> partOf;     // claims->P361->mainsnak->datavalue->value->id
    optional<uint32_t> hasPart;    // claims->P2670->mainsnak->datavalue->value->id
    bool compClass;                // claims->P2094
    bool season;                   // claims->P3450 sports season of league or competition
    bool facetOf;                  // claims->P1269
    bool series;                   // claims->P179
    optional<uint32_t> organizer;  // claims->P664->mainsnak->datavalue->value->id
    optional<uint32_t> office;     // claims->P541->mainsnak->datavalue->value->id office contested of election
    bool winner;                   // claims->P1346 winner of a competition or similar event
    bool dist;                     // claims->P3157 distance of race or other event
    bool listOf;                   // claims->P360
    bool hqLoc;                    // claims->P159 headquarters location
    //bool operator_;                // claims->P137 operator
    bool maintainer;               // claims->P126
    bool population;               // claims->P1082
    bool genre;                    // claims->P136
    bool format;                   // claims->P437 distribution format
    bool website;                  // claims->P856 official website
    bool describeUrl;              // claims->P973 described at URL

    optional<wiki_coord> coord;    // claims->P625->mainsnak->datavalue->value->longitude/latitude * coord_scale
    optional<uint32_t>   location; // claims->P276->mainsnak->datavalue->value->id
    optional<uint32_t>   street;   // claims->P669->mainsnak->datavalue->value->id
    optional<uint32_t>   admin;    // claims->P131->mainsnak->datavalue->value->id
    optional<uint32_t>   juri;     // claims->P1001->mainsnak->datavalue->value->id applies to jurisdiction
    optional<uint32_t>   country;  // claims->P17->mainsnak->datavalue->value->id

    optional<wiki_date> start; // claims->P580->mainsnak->datavalue->value->time
    optional<wiki_date> end;   // claims->P582->mainsnak->datavalue->value->time
    optional<wiki_date> date;  // claims->P585->mainsnak->datavalue->value->time

    constexpr bool valid() const noexcept
    {
        return coord || (has_time() && (location || street || admin || country));
    }

    constexpr bool has_time() const noexcept
    {
        return date || start || end;
    }

    constexpr bool has_coord_time() const noexcept
    {
        return coord && has_time();
    }

    constexpr std::pair<int32_t, int32_t> int32_time_range() const noexcept
    {
        int32_t minT = start ? start->to_int32() : 0;
        int32_t maxT = end ? end->to_int32() : 0;

        if(! minT) minT = maxT;
        if(! maxT) maxT = minT;

        if((! minT || minT > maxT) && date)
        {
            minT = maxT = date->to_int32();
        }

        DSK_ASSERT(minT && maxT);
        return {minT, maxT};
    }

    auto to_str() const
    {
        auto s = cat_as_str('Q', id, ": ", title);

        if(coord   ) append_as_str(s, "\n      coord   : ", coord->lon/coord_scale, ", ", coord->lat/coord_scale);
        if(location) append_as_str(s, "\n      location: Q", *(location));
        if(street  ) append_as_str(s, "\n      street  : Q", *(street));
        if(admin   ) append_as_str(s, "\n      adminLoc: Q", *(admin));
        if(country ) append_as_str(s, "\n      country : Q", *(country));
        if(start   ) append_as_str(s, "\n      start: ", start->to_int32());
        if(end     ) append_as_str(s, "\n      end  : ", end->to_int32());
        if(date    ) append_as_str(s, "\n      date : ", date->to_int32());

        if(classes.size())
        {
            append_str(s, "\n      Instance of: ");
            for(auto& v : classes)
            {
                append_as_str(s, 'Q', v, "|");
            }
        }

        return s;
    }
};


constexpr auto wiki_id_conv = jconv
(
    [](_unsigned_ auto d) { return cat_as_str('Q', d); },
    [](_byte_str_ auto const& s, _unsigned_ auto& d) -> error_code
    {
        auto u = str_view<char>(s);
        if(! u.starts_with('Q')) return errc::type_mismatch;
        u.remove_prefix(1);
        return from_whole_str(u, d);
    }
);

constexpr auto wiki_coord_conv = jconv
(
    [](int d) { return d / coord_scale; },
    [](double s, int32_t& d) { d = static_cast<int32_t>(s * coord_scale); }
);

constexpr auto wiki_date_conv = jconv
(
    [](wiki_date const& d) { return d.to_str(); },
    [](_byte_str_ auto const& s, wiki_date& d) { return d.from_str(s); }
);

constexpr auto wiki_qid_def = jobj("mainsnak", jobj("datavalue", jobj("value", jobj("id", wiki_id_conv))));

constexpr auto wiki_coord_def = jobj("mainsnak", jobj("datavalue", jobj("value", jobj("latitude", DSK_JSEL(lat, wiki_coord_conv),
                                                                                      "longitude", DSK_JSEL(lon, wiki_coord_conv)))));

constexpr auto wiki_date_def = jobj("mainsnak", jobj("datavalue", jobj("value", jobj("time", wiki_date_conv))));


constexpr auto wiki_item_def = jobj
(
    "type",         jexam_eq_to("item"),
    "id"          , DSK_JSEL(id, wiki_id_conv),
    "labels"      , jobj("en", jobj("value", DSK_JSEL(title))),
    "claims"      , jobj
    (
        "P31"  , DSK_JSEL(classes    , jarr_of(wiki_qid_def)),
        "P279" , DSK_JSEL(subClassOf , joptional(jarr(wiki_qid_def))),
        "P641" , DSK_JSEL(sport      , jexist),
        "P361" , DSK_JSEL(partOf     , joptional(jarr(wiki_qid_def))),
        "P2670", DSK_JSEL(hasPart    , joptional(jarr(wiki_qid_def))),
        "P2094", DSK_JSEL(compClass  , jexist),
        "P3450", DSK_JSEL(season     , jexist),
        "P1269", DSK_JSEL(facetOf    , jexist),
        "P179" , DSK_JSEL(series     , jexist),
        "P664" , DSK_JSEL(organizer  , joptional(jarr(wiki_qid_def))),
        "P541" , DSK_JSEL(office     , joptional(jarr(wiki_qid_def))),
        "P1346", DSK_JSEL(winner     , jexist),
        "P3157", DSK_JSEL(dist       , jexist),
        "P360" , DSK_JSEL(listOf     , jexist),
        "P159" , DSK_JSEL(hqLoc      , jexist),
        //"P137" , DSK_JSEL(operator_  , jexist),
        "P126" , DSK_JSEL(maintainer , jexist),
        "P1082", DSK_JSEL(population , jexist),
        "P136" , DSK_JSEL(genre      , jexist),
        "P437" , DSK_JSEL(format     , jexist),
        "P856" , DSK_JSEL(website    , jexist),
        "P973" , DSK_JSEL(describeUrl, jexist),

        "P625" , DSK_JSEL(coord   , joptional(jarr(wiki_coord_def))),
        "P276" , DSK_JSEL(location, joptional(jarr(wiki_qid_def))),
        "P669" , DSK_JSEL(street  , joptional(jarr(wiki_qid_def))),
        "P131" , DSK_JSEL(admin   , joptional(jarr(wiki_qid_def))),
        "P1001", DSK_JSEL(juri    , joptional(jarr(wiki_qid_def))),
        "P17"  , DSK_JSEL(country , joptional(jarr(wiki_qid_def))),

        "P585", DSK_JSEL(date , joptional(jarr(wiki_date_def))),
        "P580", DSK_JSEL(start, joptional(jarr(wiki_date_def))),
        "P582", DSK_JSEL(end  , joptional(jarr(wiki_date_def))),

        jnested, jcheck([](wiki_item const& t) { return t.valid(); })
    )
);


class wiki_lines : public vector<std::string_view>
{
    string _buf;

public:
    auto& buffer(this auto&& self) noexcept
    {
        return DSK_FORWARD(self)._buf;
    }

    size_t take_lines(string& buf)
    {
        buf.reserve(buf.size() + simdjson::SIMDJSON_PADDING);

        clear();
        _buf.clear();

        std::string_view dv = buf;

        for(;;)
        {
            auto pos = dv.find('\n');

            if(pos == npos)
            {
                break;
            }

            if(pos > 663)
            {
                auto ep = dv.rfind('}', pos - 1);
                
                if(ep != npos && ep > 662)
                {
                    emplace_back(dv.substr(0, ep + 1));
                }
            }

            if(auto np = dv.find('{', pos); np != npos)
            {
                pos = np;
            }
            else
            {
                ++pos;
            }

            dv.remove_prefix(pos);
        }

        if(dv.size() < buf.size())
        {
            _buf = dv;
            std::swap(_buf, buf);
        }

        return size();
    }
};


constexpr size_t fileReadBatchSize = 1*1024*1024;
res_queue<string    > decompressQueue{3};
res_queue<wiki_lines>      parseQueue{26};
res_queue<wiki_item >    writeDbQueue{126};

auto gen_queue_report(char const* name, auto& q)
{
    return cat_as_str("\n      ", name, ": enqueueWait=", with_fmt<'f', 1>(100 * q.stats().enqueue_wait_rate()), "%"
                                        ", dequeueWait=", with_fmt<'f', 1>(100 * q.stats().dequeue_wait_rate()), "%\n");
}


task<> read_file(stream_file file)
{
    perf_counter<B_, MiB> cnter("File Read", "MB");
    periodic_reporter rpt;

    char buf[fileReadBatchSize];

    for(;;)
    {
        auto cnterScope = cnter.begin_count_scope();
        auto r = DSK_WAIT file.read_some(buf);

        if(has_err(r))
        {
            if(get_err(r) == asio::error::eof)
            {
                stdout_report("File end reached.\n");
                decompressQueue.mark_end();
                break;
            }

            DSK_THROW(get_err(r));
        }

        size_t n = get_val(r);
        cnterScope.add_and_end(B_(n));
        DSK_TRY decompressQueue.enqueue(buf, n);

        rpt.report([&](){ return gen_queue_report("fileRead->decompress", decompressQueue); });
    }

    DSK_RETURN();
}


task<> decompress()
{
    perf_counter<B_, MiB> cnter("Decompress", "MB");
    periodic_reporter rpt;

    auto_decompressor decompr;

    string buf;

    for(;;)
    {
        auto r = DSK_WAIT decompressQueue.dequeue();

        if(has_err(r))
        {
            if(get_err(r) == errc::end_reached)
            {
                if(buf.size())
                {
                    buf += '\n';
                    if(wiki_lines lines; lines.take_lines(buf))
                    {
                        DSK_TRY parseQueue.enqueue(mut_move(lines));
                    }
                }

                parseQueue.mark_end();
                break;
            }

            DSK_THROW(get_err(r));
        }
        
        auto& in = get_val(r);

        auto cnterScope = cnter.begin_count_scope();
        auto u = DSK_TRY_SYNC decompr.append_multi(buf, in);
        cnterScope.add_and_end(B_(u.nOut));

        if(u.nOut/* && buf.size() > 3 * 660000*/)
        {                           //^^^^^^: assumed average line size
            if(wiki_lines lines; lines.take_lines(buf))
            {
                DSK_TRY parseQueue.enqueue(mut_move(lines));
            }
        }

        rpt.report([&](){ return gen_queue_report("decompress->parse", parseQueue); });
    }

    DSK_RETURN();
}


mt_perf_counter<B_, MiB> parseCnter("Parse", "MB");
mt_periodic_reporter itemQueueRpt;

task<> parse_json()
{
    simdjson_reader jreader;
    vector<wiki_item> items;

    for(;;)
    {
        auto r = DSK_WAIT parseQueue.dequeue();

        if(has_err(r))
        {
            if(get_err(r) == errc::end_reached)
            {
                break;
            }

            DSK_THROW(get_err(r));
        }

        wiki_lines& lines = get_val(r);

        auto cnterScope = parseCnter.begin_count_scope();
        for(auto& l : lines)
        {
            wiki_item item;

            auto r = jread_whole(wiki_item_def, jreader,
                                 simdjson::padded_string_view(l, l.size() + simdjson::SIMDJSON_PADDING),
                                 item);  //^^^^^^^^^^^^^^^^^^: should have reserved enough space in wiki_lines::take_lines().

            if(! has_err(r))
            {
                items.emplace_back(mut_move(item));
            }
        }
        cnterScope.add_and_end(B_(lines.buffer().size()));

        {
            auto r = writeDbQueue.force_enqueue_range(items);

            if(has_err(r) && get_err(r) != errc::end_reached)
            {
                DSK_THROW(get_err(r));
            }
        }

        items.clear();

        itemQueueRpt.report([&](){ return gen_queue_report("parse->db", writeDbQueue); });
    }

    DSK_RETURN();
}

struct wiki_location
{
    optional<wiki_coord> coord;
    vector<wiki_item> items; // items use this loc.
};

task<> populate_db(sqlite3_conn db)
{
    perf_counter<Ones> cnter("Database", "items");

    auto insert = DSK_TRY_SYNC db.prepare_persistent(
    R"(
        INSERT INTO items(id, minX, maxX, minY, maxY, minT, maxT, title, class)
                   VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    auto insertBroken = DSK_TRY_SYNC db.prepare_persistent(
    R"(
        INSERT INTO broken_items(id, minX, maxX, minY, maxY, minT, maxT, title, class)
                          VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    unstable_unordered_map<uint32_t, size_t> classStats;

    auto do_insert = [&](wiki_item& item)
    {
        DSK_ASSERT(item.has_coord_time());

        if(item.sport || item.compClass || item.season || item.winner || item.dist
           || item.listOf || item.hqLoc || item.maintainer || item.population
           || item.genre || item.format)
        {
            return 0;
        }

        if(item.subClassOf && is_oneof(*item.subClassOf, 22231119u, 60181400u)) // cycling race class
        {
            return 0;
        }

        if(has_ignored_class(item.classes))
        {
            return 0;
        }

        uint32_t class_ = item.classes[0];

        if(item.facetOf || item.series)
        {
            if(class_ == 625994 || class_ == 2761147) // convention, meeting
            {
                return 0;
            }
        }

        if(item.hasPart && is_oneof(*item.hasPart, 15966540u, 6508605u)) // local election, leaders' debate
        {
            return 0;
        }
        
        if(item.partOf)
        {
            if(item.office || item.juri || class_ == 76853179) // group of elections
            {
                return 0;
            }

            if(class_ == 1656682) // event
            {
                if(has_ignored_class(*item.partOf))
                {
                    return 0;
                }
            }
        }

        if(item.office)
        {
            if(item.country != item.juri) // only country level election
            {
                return 0;
            }

            //// United States senator, council member, Member of the European Parliament, mayor
            //if(is_oneof(*item.office, 4416090u, 708492u, 27169u, 30185u))
            //{
            //    return 0;
            //}

            class_ = 40231; // public election
        }

        if(class_ == 625994 || class_ == 1656682) // convention, event
        {
            if(item.partOf || item.website || item.describeUrl)
            {
                return 0;
            }
            else if(item.organizer && has_ignored_class(*item.organizer))
            {
                return 0;
            }
        }

        auto[minT, maxT] = item.int32_time_range();

        if(class_ == 182832 && minT > 19260101) // concert
        {
            return 0;
        }
        else if(class_ == 174782 && minT > 16260101) // square
        {
            return 0;
        }
        else if(class_ == 4989906 && minT > 19160101) // monument
        {
            return 0;
        }
        else if(is_oneof(class_, 1190554u, 12890393u, 2334719u, 1656682u)) // occurrence, incident, legal case, event
        {
            if(item.classes.size() > 1)
            {
                class_ = item.classes[1];
            }
            else if(class_ == 2334719) // legal case
            {
                return 0;
            }
        }

        class_ = remap_class(class_);

        auto r = minT > maxT ?
            insertBroken.exec(item.id,
                              item.coord->lon, item.coord->lon,
                              item.coord->lat, item.coord->lat,
                              minT, maxT,
                              item.title, class_)
            : insert.exec(item.id,
                          item.coord->lon, item.coord->lon,
                          item.coord->lat, item.coord->lat,
                          minT, maxT,
                          item.title, class_);

        if(has_err(r))
        {
            stdout_report("\n Insert to db failed: ", get_err_msg(r),
                          "\n Item: ", item.to_str(), '\n');
            return 0;
        }

        ++classStats[class_];
        return 1;
    };

    unstable_unordered_map<uint32_t, wiki_location> locs;

    auto add_item_loc = [&](wiki_item& item, uint32_t locId)
    {
        wiki_location& loc = locs[locId];

        if(! loc.coord)
        {
            loc.items.emplace_back(mut_move(item));
            return true;
        }

        item.coord = loc.coord;
        return false;
    };

    vector<wiki_item> items;

    for(;;)
    {
        auto r = DSK_WAIT writeDbQueue.dequeue();

        if(has_err(r))
        {
            if(get_err(r) == errc::end_reached)
            {
                break;
            }

            DSK_THROW(get_err(r));
        }

        items.emplace_back(mut_move(get_val(r)));
        
        {
            auto r = writeDbQueue.force_dequeue_all(items);

            if(has_err(r) && get_err(r) != errc::end_reached)
            {
                DSK_THROW(get_err(r));
            }
        }

        auto cnterScope = cnter.begin_count_scope();
        size_t nInserted = 0;
        {
            DSK_CLEANUP_SCOPE_BEGIN();
            auto trxn = DSK_TRY db.make_transaction();

            for(wiki_item& item : items)
            {
                if(! item.has_coord_time())
                {
                    if(item.coord)
                    {
                        locs[item.id].coord = item.coord;
                        continue;
                    }

                    if(item.location)
                    {
                        if(add_item_loc(item, *item.location))
                            continue;
                    }
                    else if(item.street)
                    {
                        if(add_item_loc(item, *item.street))
                            continue;
                    }
                    else if(item.admin)
                    {
                        if(add_item_loc(item, *item.admin))
                            continue;
                    }
                    else if(item.juri)
                    {
                        if(add_item_loc(item, *item.juri))
                            continue;
                    }
                    else // if(item.country)
                    {
                        if(add_item_loc(item, *item.country))
                            continue;
                    }
                }

                nInserted += do_insert(item);
            }

            DSK_TRY_SYNC trxn.commit();
            DSK_CLEANUP_SCOPE_END();
        }
        cnterScope.add_and_end(Ones(nInserted));

        items.clear();
    }

    stdout_report("\n\n=========> Post process items <=========\n\n");
    {
        DSK_CLEANUP_SCOPE_BEGIN();
        auto trxn = DSK_TRY db.make_transaction();

        size_t totalCnt = 0;
        size_t batchCnt = 0;

        for(auto&[id, loc] : locs)
        {
            if(! loc.coord)
            {
                continue;
            }

            for(wiki_item& item : loc.items)
            {
                item.coord = loc.coord;
                
                if(do_insert(item))
                {
                    ++totalCnt;
                    ++batchCnt;
                    
                    if(batchCnt >= 126)
                    {
                        DSK_TRY_SYNC trxn.commit_and_begin();
                        batchCnt = 0;
                    }
                }
            }
        }

        DSK_TRY_SYNC trxn.commit();
        stdout_report('\n', totalCnt, " items inserted\n");
        DSK_CLEANUP_SCOPE_END();
    }

    stdout_report("\n\n=========> Insert classes <=========\n\n");

    auto insertClass = DSK_TRY_SYNC db.prepare_persistent(
        "INSERT INTO classes(id, cnt) VALUES(?, ?)");

    {
        DSK_CLEANUP_SCOPE_BEGIN();
        auto trxn = DSK_TRY db.make_transaction();

        size_t totalCnt = 0;

        for(auto&[id, cnt] : classStats)
        {
            auto r = insertClass.exec(id, cnt);

            if(has_err(r))
            {
                stdout_report("\n    Insert class to db failed: ", get_err_msg(r),
                              "\n        P", id, ", cnt: ", cnt, '\n');
            }
            else
            {
                ++totalCnt;
            }
        }

        DSK_TRY_SYNC trxn.commit();
        stdout_report('\n', totalCnt, " classes inserted\n");
        DSK_CLEANUP_SCOPE_END();
    }

    DSK_RETURN();
}


int main(/*int argc, char* argv[]*/)
{
    stream_file file;

    //auto fr = file.open("E:\\latest-all.json.gz", file_base::read_only);
    auto fr = file.open("Z:\\latest-all.json.zstd", file_base::read_only);

    if(has_err(fr))
    {
        stdout_report("Failed to open file: ", get_err_msg(fr));
        return -1;
    }

    sqlite3_conn db;

    auto dr = sync_wait([&]() -> task<>
    {
        DSK_TRY_SYNC db.open("E:\\wiki_hist.sqlite");

        DSK_TRY_SYNC db.set_locking_mode(locking_mode_exclusive);
        DSK_TRY_SYNC db.set_journal_mode(journal_mode_truncate);
        DSK_TRY_SYNC db.set_synchronous_mode(synchronous_mode_off);
        DSK_TRY_SYNC db.set_temp_store(temp_store_memory);
        DSK_TRY_SYNC db.set_cache_size(MiB(66));
        DSK_TRY_SYNC db.exec_multi(R"(
            CREATE VIRTUAL TABLE IF NOT EXISTS items USING rtree_i32(
                id, -- QID
                minX, maxX, -- lon
                minY, maxY, -- lat
                minT, maxT, -- time
                +title TEXT,  -- type affinity and constraints are ignored, so just for document purpose
                +class INTEGER -- first class of P31(instance of)
            );

            CREATE TABLE IF NOT EXISTS classes(
                id   INTEGER PRIMARY KEY, -- QID
                cnt  INTEGER,
                name TEXT
            );

            CREATE TABLE IF NOT EXISTS broken_items(
                id   INTEGER PRIMARY KEY,
                minX INTEGER, maxX INTEGER,
                minY INTEGER, maxY INTEGER,
                minT INTEGER, maxT INTEGER,
                title TEXT,
                class INTEGER
            );
        )");

        DSK_RETURN();
    }());

    if(has_err(dr))
    {
        stdout_report("Failed to create/open db: ", get_err_msg(dr));
        return -1;
    }


    DSK_DEFAULT_IO_SCHEDULER.start();

    auto r = sync_wait(until_first_failed
    (
        run_on(DSK_DEFAULT_IO_SCHEDULER, read_file(mut_move(file))),
        run_on(DSK_DEFAULT_IO_SCHEDULER, decompress()),
        []() -> task<>
        {
            //DSK_TRY until_all_succeeded<2>(/*2, */[](){ return
            //            run_on(DSK_DEFAULT_IO_SCHEDULER, parse_json()); });
            DSK_TRY run_on(DSK_DEFAULT_IO_SCHEDULER, parse_json());
            writeDbQueue.mark_end();
            DSK_RETURN();
        }(),
        run_on(DSK_DEFAULT_IO_SCHEDULER, populate_db(mut_move(db)))
    ));

    if(! has_err(r))
    {
        stdout_report("\n\nProcessing failed: ",
                      visit_var(get_val(r), [](auto& r){ return get_err_msg(r); }));
        return -1;
    }

    return 0;
}