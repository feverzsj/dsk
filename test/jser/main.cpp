#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

// #define SIMDJSON_VERBOSE_LOGGING 1
#include <dsk/jser/dsl.hpp>
#include <dsk/jser/json/simdjson.hpp>
#include <dsk/jser/json/writer.hpp>
#include <dsk/jser/msgpack/reader.hpp>
#include <dsk/jser/msgpack/writer.hpp>
#include <dsk/util/map.hpp>
#include <dsk/util/deque.hpp>
#include <dsk/util/vector.hpp>


using namespace dsk;

template<auto MakeWriter, _jreader_ Reader>
void do_test(auto&& def, auto const& tarVal, char const* subject)
{
    string buf;
    CHECK_NOTHROW_MESSAGE(jwrite(def, tarVal, MakeWriter(buf)), subject, " Write");

    Reader reader;
    DSK_NO_CVREF_T(tarVal) val{};
    auto r = jread_whole(def, reader, buf, val);
    CHECK_MESSAGE(! has_err(r), subject, " Read: ", get_err_msg(r));
    CHECK_MESSAGE(val == tarVal, subject, " results not equal");

    string rbuf;
    jwrite_range(def, std::array{val, val, val}, MakeWriter(rbuf));

    auto test_read_range = [&](auto useGeneral)
    {
        char const* kind = useGeneral ? " General" : "Specialized";

        vector<DSK_NO_CVREF_T(tarVal)> vals;
        r = jread_range_whole<useGeneral>(def, reader, rbuf, vals);

        CHECK_MESSAGE(! has_err(r), subject, kind, "read range ", get_err_msg(r));
        CHECK_MESSAGE(vals.size() == 3, subject, kind, "read range wrong count");

        for(auto& v : vals)
        {
            CHECK_MESSAGE(v == tarVal, subject, kind, ": read range results not equal");
        }
    };

    test_read_range(constant_c<juse_general_foreach>);

    if constexpr(_same_as_<Reader, simdjson_reader>)
    {
        test_read_range(constant_c<jdont_use_general_foreach>);
    }
}

void test_all(auto&& def, auto const& tarVal)
{
    do_test<[](auto& b) { return make_json_writer(b); }, simdjson_reader>(def, tarVal, "simdjson");
    do_test<[](auto& b) { return make_msgpack_writer(b); }, msgpack_reader>(def, tarVal, "msgpack");
    do_test<[](auto& b) { return make_msgpack_writer<msgpack_opt_little_endian>(b); }, msgpack_reader_t<msgpack_opt_little_endian>>(def, tarVal, "msgpack_le");
    do_test<[](auto& b) { return make_msgpack_writer<msgpack_opt_packed_arr>(b); }, msgpack_reader_t<msgpack_opt_packed_arr>>(def, tarVal, "msgpack_parr");
    do_test<[](auto& b) { return make_msgpack_writer<msgpack_opt_le_parr>(b); }, msgpack_reader_t<msgpack_opt_le_parr>>(def, tarVal, "msgpack_le_parr");
}


TEST_CASE("jser")
{
    SUBCASE("dsl")
    {
        struct q_st
        {
            bool operator==(q_st const&) const = default;

            string str;
            double d;
            optional<int> opti;
        };

        constexpr auto Q_J = jobj
        (
            "str", DSK_JSEL(str),
            "d", DSK_JSEL(d),
            "opti", DSK_JSEL(opti)
        );

        using val_var = std::variant<std::monostate, int, bool, string, q_st, vector<q_st>>;
        constexpr auto ValVar_J = jval_variant(jauto, jauto, jauto, Q_J, jarr_of(Q_J));

        struct a_st
        {
            bool operator==(a_st const&) const = default;

            bool b;
            int i;
            unsigned u;
            float f;
            double d;
            string str;
            string strNoescape;
            int iarr[6];
            string sarr[6];
            tuple<int, double, bool, string> tarr0;
            std::tuple<int, double, string, string, q_st> tarr1;
            tuple<int, std::pair<double, string>, string, string> tarr2;
            std::tuple<std::array<int, 1>, std::pair<double, string>, string> tarr3;
            vector<float> farr;
            deque<tuple<int, string>> darr;

            optional<int> opti;
            optional<double> optd;
            optional<string> optStr;
            optional<string> optStrNsc;
            optional<q_st> optq;
            tuple<optional<int>, bool, double> topt0;
            tuple<bool, optional<int>, optional<string>, optional<q_st>> topt1;

            using prim_var = std::variant<std::monostate, int64_t, double, bool, string, vector<int>>;
            prim_var var0;
            prim_var var1;
            prim_var var2;
            prim_var var3;
            prim_var var4;

            val_var valVar0;
            val_var valVar1;
            val_var valVar2;

            using nested_var = std::variant<std::monostate, int, bool, q_st>;
            nested_var varObj0;
            nested_var varObj1;

            int32_t binI;
            uint32_t binU;
            float binD;
            string binS;

            int t0;
            float t1;
            string t2;

            // parr2/parr6 : jpacked_arr_of<int64_t, 2>(), only first 2 elms is used,
            // val init assures tarVal and val are comparable.
            struct val_inited_int64_arr3 : std::array<int64_t, 3>
            {
                val_inited_int64_arr3(int64_t a = 0, int64_t b = 0, int64_t c = 0)
                    : std::array<int64_t, 3>{a,b,c}
                {}
            };
            struct val_inited_int32_arr3 : std::array<int32_t, 3>
            {
                val_inited_int32_arr3(int32_t a = 0, int32_t b = 0, int32_t c = 0)
                    : std::array<int32_t, 3>{a,b,c}
                {}
            };

            vector<uint16_t> parr0;
            vector<std::array<float, 2>> parr1;
            vector<val_inited_int64_arr3> parr2;
            vector<std::array<double, 3>> parr3;
            std::array<uint32_t, 2> parr5[3];
            val_inited_int32_arr3 parr6[3];
        };

        constexpr auto A_J = jobj
        (
            jkey<-1u>("verifier0"), jexam_eq_to("for exam"),
            jkey<-2u>("verifier1"), jexam_eq_to(6.26),
            jkey<-3u>("verifier2"), jexam("\\u0266\\u0263",
                                          [](auto& d){ return d == "\\u0266\\u0263"; },
                                          jnoescape),
            jkey<1>("b"), DSK_JSEL(b, jcheck_eq_to(true)),
            jkey<2>("i"), DSK_JSEL(i),
            jkey<3>("u"), DSK_JSEL(u),
            jkey<4>("f"), DSK_JSEL(f),
            jkey<5>("d"), DSK_JSEL(d),
            jkey<6>("str"), DSK_JSEL(str),
            jkey<7>("strNoescape"), DSK_JSEL(strNoescape, jnoescape),
            jkey<8>("iarr"), DSK_JSEL(iarr),
            jkey<9>("sarr"), DSK_JSEL(sarr),
            jkey<10>("tarr0"), DSK_JSEL(tarr0),
            jkey<11>("tarr1"), DSK_JSEL(tarr1, jtuple(jcur, jcur, jcur, jnoescape, Q_J)),
            jkey<12>("tarr2"), DSK_JSEL(tarr2, jtuple<3>(jcur, jcur, jnoescape)),
            jkey<13>("tarr3"), DSK_JSEL(tarr3, jtuple(jcur, jcur, jnoescape)),
            jkey<14>("farr"), DSK_JSEL(farr),
            jkey<15>("darr"), DSK_JSEL(darr, jcheck([](auto& darr){ return darr.size() == 2; },
                                                    jarr_of<2>(jtuple(jcur, jnoescape)))),
            jkey<16>("opti"), DSK_JSEL(opti),
            jkey<17>("optd"), DSK_JSEL(optd),
            jkey<18>("optStr"), DSK_JSEL(optStr),
            jkey<19>("optStrNsc"), DSK_JSEL(optStrNsc, joptional(jnoescape)),
            jkey<20>("optq"), DSK_JSEL(optq, joptional(Q_J)),
            jkey<21>("topt0"), DSK_JSEL(topt0),
            jkey<22>("topt1"), DSK_JSEL(topt1, jtuple(jcur, jcur, joptional(jnoescape), joptional(Q_J))),
            jkey<23>("var0"), DSK_JSEL(var0),
            jkey<24>("var1"), DSK_JSEL(var1),
            jkey<25>("var2"), DSK_JSEL(var2),
            jkey<26>("var3"), DSK_JSEL(var3),
            jkey<36>("var4"), DSK_JSEL(var4),
            jkey<66>("valVar0"), DSK_JSEL(valVar0, ValVar_J),
            jkey<62>("valVar1"), DSK_JSEL(valVar1, ValVar_J),
            jkey<63>("valVar2"), DSK_JSEL(valVar2, ValVar_J),
            jkey<27>("varObj0"), DSK_JSEL(varObj0, jobj
            (
                jkey<1>("type"), jconv([](auto& varObj0)
                                       {
                                           DSK_ASSERT(varObj0.index());
                                           return varObj0.index();
                                       },
                                       [](auto i, auto& varObj0)
                                       {
                                           if(0 < i && i < std::variant_size_v<DSK_NO_CVREF_T(varObj0)>)
                                           {
                                               assure_var_holds(varObj0, i);
                                               return true;
                                           }
                                       
                                           return false;
                                       }),
                jnested, jnested_variant(jobj(jkey<2>("h1"), jcur),
                                         jobj(jkey<3>("h2"), jcur),
                                         jobj(jkey<4>("h3"), Q_J))
            )),
            jkey<28>("varObj1"), DSK_JSEL(varObj1, jobj
            (
                "type", jconv([](auto& varObj1)
                              {
                                  switch(varObj1.index())
                                  {
                                      case 1: return "h1";
                                      case 2: return "h2";
                                      case 3: return "h3";
                                  }
                                  DSK_ASSERT(false);
                                  DSK_UNREACHABLE();
                                  return "invalid index";
                              },
                              [](auto&& t, auto& varObj1)
                              {
                                  size_t i = 0;
                                       if(t == "h1") i = 1;
                                  else if(t == "h2") i = 2;
                                  else if(t == "h3") i = 3;
                                  else               return false;
                              
                                  assure_var_holds(varObj1, i);
                                  return true;
                              }),
                jnested, jnested_variant(jobj("h1", jcur),
                                         jobj("h2", jcur),
                                         jobj("h3", Q_J))
            )),

            jkey<29>("binI"), DSK_JSEL(binI, jbinary),
            jkey<30>("binU"), DSK_JSEL(binU, jbinary),
            jkey<31>("binD"), DSK_JSEL(binD, jbinary),
            jkey<32>("binS"), DSK_JSEL(binS, jbinary),

            jkey<33>("tarrs"), jarr(DSK_JSEL(t2), DSK_JSEL(t0), DSK_JSEL(t1)),

            jkey<34>("parr0"), DSK_JSEL(parr0, jpacked_arr_of<uint16_t>()),
            jkey<35>("parr1"), DSK_JSEL(parr1, jpacked_arr_of<float, 2>()),
            jkey<96>("parr2"), DSK_JSEL(parr2, jpacked_arr_of<int64_t, 2>()),
            jkey<38>("parr3"), DSK_JSEL(parr3, jpacked_arr_of<double>(jtuple<3>())),
            jkey<39>("parr5"), DSK_JSEL(parr5, jpacked_arr_of<uint32_t, 2>()),
            jkey<40>("parr6"), DSK_JSEL(parr6, jpacked_arr_of<int64_t, 2>())
        );

        a_st const tarVal = {
            .b = true,
            .i = -1,
            .u = 1u,
            .f = 1.23456f,
            .d = 1.23456789,
            .str = "\u0266\u0263",
            .strNoescape = "\\u0266\\u0263",
            .iarr = { 1, 2, 3, 4, 5, 6 },
            .sarr = { "1", "2", "3", "4", "5", "6" },
            .tarr0 = {-1, 2.3, true, "\u0266\u0263"},
            .tarr1 = {-1, 2.3, "\u0266\u0263", "\\u0266\\u0263", q_st{"\u0262\u0263", 1.26, 6}},
            .tarr2 = {-1, std::pair(2.3, "\u0266\u0263"), "\\u0266\\u0263"},
            .tarr3 = {std::array{-1}, std::pair(2.3,"\u0266\u0263"),"\\u0266\\u0263"},
            .farr = { 1.26, 2.21, 3.69, 4.02, 5.8, 6.6 },
            .darr = {{1, "\\u0266\\u0263"}, {2, "\\u0226\\u0262"}},

            .opti = nullopt,
            .optd = 6.26,
            .optStr = "\u0266\u0263",
            .optStrNsc = "\\u0226\\u0262",
            .optq = q_st{"\u0262\u0263", 1.26, 6},
            .topt0 = {nullopt, true, 6.26},
            .topt1 = {true, 266, "\\u0226\\u0262"},

            .var0 = std::monostate{},
            .var1 = 12.66,
            .var2 = "var2",
            .var3 = true,
            .var4 = vector{2, 6},

            .valVar0 = 6,
            .valVar1 = q_st{"\u0262\u0263", 1.26, 6},
            .valVar2 = vector{q_st{"\u0262\u0263", 2.26, 26}, q_st{"\u0262\u0263", 6.26, 66},},

            .varObj0 = q_st{"\u0262\u0263", 1.26, 6},
            .varObj1 = true,

            .binI = 2666266,
            .binU = 2666226666,
            .binD = 666666.26666,
            .binS = "626aweqwe",

            .t0 = 26,
            .t1 = 6.6,
            .t2 = "266sfraeqwe",

            .parr0 = {1, 2, 3, 4, 5, 6},
            .parr1 = {{1, 2}, {3, 4}, {5, 6}},
            .parr2 = {{1, 2}, {3, 4}, {5, 6}},
            .parr3 = {{1, 2, 3}, {4, 5, 6}},
            .parr5 = {{1, 2}, {3, 4}, {5, 6}},
            .parr6 = {{1, 2}, {3, 4}, {5, 6}}
        };

        test_all(A_J, tarVal);
    } // SUBCASE("dsl")


    SUBCASE("geojson")
    {
        using point_t = std::array<double, 2>;
        using linestring_t = vector<point_t>;
        using polygon_t = vector<linestring_t>;

        struct multi_point_t: vector<point_t>
        {
            //using vector<point_t>::vector; doesn't work
            using vector::vector;
        };

        struct multi_linestring_t: vector<linestring_t>
        {
            //using vector<linestring_t>::vector;
            using vector::vector;
        };

        using multi_polygon_t = vector<polygon_t>;

        struct geometry_collection_t;

        using geometry_t = std::variant<std::monostate,
                                        point_t, linestring_t, polygon_t,
                                        multi_point_t, multi_linestring_t, multi_polygon_t,
                                        geometry_collection_t>;

        struct geometry_collection_t : vector<geometry_t> // the container type must support incomplete type
        {
            //using vector<geometry_t>::vector;
            using vector::vector;
        };

        using property_map = map<string, std::variant<std::monostate, int64_t, double, bool, string>>;

        struct feature_t
        {
            geometry_t geom;
            property_map props{};

            constexpr bool operator==(feature_t const&) const = default;
        };

        using feature_coll_t = deque<feature_t>;


        constexpr auto PointSeq = jpacked_arr_of<double, 2>();
        constexpr auto Polygon  = jarr_of(PointSeq);

        constexpr auto Geomerty = jobj<"Geomerty">
        (
            "type", jconv([](auto& var)
                          {
                              switch(var.index())
                              {
                                  case 1: return "Point";
                                  case 2: return "LineString";
                                  case 3: return "Polygon";
                                  case 4: return "MultiPoint";
                                  case 5: return "MultiLineString";
                                  case 6: return "MultiPolygon";
                                  case 7: return "GeometryCollection";
                              }
                              DSK_ASSERT(false);
                              DSK_UNREACHABLE();
                              return "invalid index";
                          },
                          [](auto&& t, auto& var)
                          {
                              size_t i = 0;
                          
                                   if(t == "Point"             ) i = 1;
                              else if(t == "LineString"        ) i = 2;
                              else if(t == "Polygon"           ) i = 3;
                              else if(t == "MultiPoint"        ) i = 4;
                              else if(t == "MultiLineString"   ) i = 5;
                              else if(t == "MultiPolygon"      ) i = 6;
                              else if(t == "GeometryCollection") i = 7;
                              else return false;
                          
                              assure_var_holds(var, i);
                              return true;
                          }),
            jnested, jnested_variant(jobj("coordinates", jcur), // Point
                                     jobj("coordinates", PointSeq), // LineString
                                     jobj("coordinates", Polygon), // Polygon
                                     jobj("coordinates", PointSeq), // MultiPoint
                                     jobj("coordinates", jarr_of(PointSeq)), // MultiLineString
                                     jobj("coordinates", jarr_of(Polygon)), // MultiPolygon
                                     jobj("geometries" , jarr_of(jref<"Geomerty">)) // GeometryCollection
            )
        );

        constexpr auto Feature = jobj
        (
            "type"      , jexam_eq_to("Feature"),
            "geometry"  , DSK_JSEL(geom, Geomerty),
            "properties", DSK_JSEL(props, jmap_obj())
        );

        constexpr auto FeatureCollection = jobj
        (
            "type"    , jexam_eq_to("FeatureCollection"),
            "features", jarr_of(Feature)
        );

        feature_coll_t const tarVal =
        {
            feature_t{
                point_t{30.0, 10.0}, {{"prop0", "value0"}, {"prop1", 1}}
            },
            feature_t{
                linestring_t{{30.0, 10.0}, {10.0, 30.0}, {40.0, 40.0}}, {{"prop0", true}, {"prop1", 6.26}}
            },
            feature_t{
                polygon_t{
                    {{35.0, 10.0}, {45.0, 45.0}, {15.0, 40.0}, {10.0, 20.0}, {35.0, 10.0}},
                    {{20.0, 30.0}, {35.0, 35.0}, {30.0, 20.0}, {20.0, 30.0}}
                },
                {{"u", 26.6}, {"val", 626}, {"b", false}}
            },
            feature_t{
                multi_point_t{{10.0, 40.0}, {40.0, 30.0}, {20.0, 20.0}, {30.0, 10.0}}
            },
            feature_t{
                multi_linestring_t{
                    {{10.0, 10.0}, {20.0, 20.0}, {10.0, 40.0}},
                    {{40.0, 40.0}, {30.0, 30.0}, {40.0, 20.0}, {30.0, 10.0}}
                },
                {{"prop", "test"}, {"d", 6.2}, {"e", 26.26}}
            },
            feature_t{
                multi_polygon_t{
                    {
                        {{10.0, 40.0}, {20.0, 20.0}, {60.0, 20.0}, {10.0, 40.0}}
                    },
                    {
                        {{35.0, 10.0}, {45.0, 45.0}, {15.0, 40.0}, {10.0, 20.0}, {35.0, 10.0}},
                        {{20.0, 30.0}, {35.0, 35.0}, {30.0, 20.0}, {20.0, 30.0}}
                    }
                }
            },
            feature_t{
                geometry_collection_t{
                    point_t{30.0, 10.0},
                    linestring_t{{30.0, 10.0}, {10.0, 30.0}, {40.0, 40.0}},
                    polygon_t{
                        {{35.0, 10.0}, {45.0, 45.0}, {15.0, 40.0}, {10.0, 20.0}, {35.0, 10.0}},
                        {{20.0, 30.0}, {35.0, 35.0}, {30.0, 20.0}, {20.0, 30.0}}
                    },
                    geometry_collection_t{
                        point_t{30.0, 10.0}
                    },
                    geometry_collection_t{
                        geometry_collection_t{point_t{30.0, 10.0}}
                    }
                }
            }
        };

        test_all(FeatureCollection, tarVal);
    } // SUBCASE("geojson")

    SUBCASE("wiki")
    {
        string input = R"qweasd(
{
   "type":"item",
   "id":"Q266",
   "labels":{
      "en":{
         "language":"en",
         "value":"some title",
        "timezone": 0
      }
   },
   "descriptions":{
      "en":{
         "language":"en",
         "value":"some description",
         "timezone": 0
      }
   },
   "claims":{
      "P625":[
         {
            "mainsnak":{
               "datavalue":{
                  "value":{
                     "latitude":26.26,
                     "longitude":126.66,
                     "timezone": 6
                  }
               }
            }
         },
        126,
        {"timezone": 0, "language":"en"}
      ],
      "P585":[
         {
            "mainsnak":{
               "datavalue":{
                  "value":{
                     "time":"+2016-06-16T00:00:00Z"
                  }
               }
            }
         }
      ],
      "P580":[
         {
            "mainsnak":{
               "datavalue":{
                  "value":{
                     "time":"+2016-06-16T00:00:00Z"
                  }
               }
            }
         }
      ],
      "P582":[
         {
            "mainsnak":{
               "datavalue":{
                  "value":{
                     "time":"+2016-06-26T00:00:00Z"
                  }
               }
            }
         }
      ]
   }
}
)qweasd";

        struct wiki_item
        {
            string id;// "Qnnnnn"
            string title; // labels->en->value
            string descr; // descriptions->en->value
            double lon, lat; // claims->P625->mainsnak->datavalue->value->longitude/latitude
            optional<string> date; // claims->P585->mainsnak->datavalue->value->time
            optional<string> start;// claims->P580->mainsnak->datavalue->value->time
            optional<string> end;  // claims->P582->mainsnak->datavalue->value->time
        };

        constexpr auto wiki_date_def = jobj("mainsnak", jobj("datavalue", jobj("value", jobj("time", jcur))));

        constexpr auto wiki_item_def = jobj
        (
            "id"          , DSK_JSEL(id),
            "type",         jexam_eq_to("item"),
            "labels"      , jobj("en", jobj("value", DSK_JSEL(title))),
            "descriptions", jobj("en", jobj("value", DSK_JSEL(descr))),
            "claims"      , jobj
            (
                "P625", jarr(jobj("mainsnak", jobj("datavalue", jobj("value", jobj("latitude", DSK_JSEL(lat),
                                                                                   "longitude", DSK_JSEL(lon)))))),
                "P585", DSK_JSEL(date , joptional(jarr(wiki_date_def))),
                "P580", DSK_JSEL(start, joptional(jarr(wiki_date_def))),
                "P582", DSK_JSEL(end  , joptional(jarr(wiki_date_def))),

                jnested, jcheck([](wiki_item const& t) { return t.date || (t.start && t.end); })
            )
        );

        wiki_item item;
        auto r = jread_whole(wiki_item_def, simdjson_reader(), input, item);
        CHECK(! has_err(r));
        CHECK(item.id == "Q266");
        CHECK(item.title == "some title");
        CHECK(item.descr == "some description");
        CHECK(item.lat == 26.26);
        CHECK(item.lon == 126.66);
        CHECK(item.date == "+2016-06-16T00:00:00Z");
        CHECK(item.start == "+2016-06-16T00:00:00Z");
        CHECK(item.end == "+2016-06-26T00:00:00Z");
    } // SUBCASE("wiki")

} // TEST_CASE("jser")
