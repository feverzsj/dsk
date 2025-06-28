#pragma once

#include <dsk/err.hpp>
#include <dsk/expected.hpp>
#include <dsk/util/buf.hpp>
#include <dsk/util/str.hpp>
#include <dsk/util/vector.hpp>
#include <dsk/charset/bom.hpp>
#include <dsk/charset/ascii.hpp>
#include <gumbo.h>
#include <memory>

#if __has_include(<unicode/ucnv.h>)
    #include <dsk/charset/icu.hpp>
#endif


namespace dsk{


enum class gumbo_errc
{
    parse_failed = 1,
};


class gumbo_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "gumbo"; }

    std::string message(int condition) const override
    {
        switch(static_cast<gumbo_errc>(condition))
        {
            case gumbo_errc::parse_failed : return "parse failed";
        }

        return "undefined";
    }
};

inline constexpr gumbo_err_category g_gumbo_errc_cat;


} // namespace dsk


DSK_REGISTER_ERROR_CODE_ENUM(dsk, gumbo_errc, g_gumbo_errc_cat)


namespace dsk{


inline std::string_view _g_to_strv(GumboStringPiece const& s) noexcept
{
    return {s.data, s.length};
}


GumboTag _g_to_tag(_char_str_ auto const& s) noexcept
{
    return gumbo_tagn_enum(str_data(s), static_cast<unsigned>(str_size(s)));
}

inline GumboTag _g_to_tag(GumboTag s) noexcept
{
    return s;
}


class gumbo_element;
class gumbo_text;
class gumbo_elm_coll;


/// selectors

struct gumbo_tag_sel
{
    GumboTag tag;

    constexpr explicit gumbo_tag_sel(GumboTag t)
        : tag(t)
    {
        DSK_ASSERT(tag != GUMBO_TAG_UNKNOWN);
    }

    constexpr explicit gumbo_tag_sel(_char_str_ auto const& t)
        : tag(_g_to_tag(t))
    {
        DSK_ASSERT(tag != GUMBO_TAG_UNKNOWN);
    }

    bool match(gumbo_element const& e) const noexcept;
};


// "tag.class #id attr1=value attr2"
// tag must present first. To match any tag, use "*". Other parts are all attrs, and can be in any order.
// attr shorthand: .someclass => class=someclass, #someid => id=someid
// attr without value will match any value (only care the presence of attr)
// ".# "(including space) are valid separators, unless it's quoted(either single or double, if one is used, the other can present inside)
// a XML attribute value may contain any character except '<' and '&', or the quote character('and") used around the value.
struct gumbo_simple_sel
{
    GumboTag tag = GUMBO_TAG_LAST;
    vector<std::pair<string, string>2> attrs;

    gumbo_simple_sel() = default;

    explicit gumbo_simple_sel(std::string_view const& s)
    {
        if(! parse(s))
            throw std::runtime_error("gumbo_simple_sel: failed to parse");
    }

    explicit gumbo_simple_sel(GumboTag tag_, _char_str_ auto const&... attrs_)
        : tag(tag_)
    {
        add_attrs(attrs_...);
    }

    void add_attrs(_char_str_ auto const& k, _char_str_ auto const& v, _char_str_ auto const&... attrs_)
    {
        attrs.emplace_back(k, v);
        add_attrs(attrs_...);
    }

    void add_attrs() noexcept {}

    bool match_any_tag() const noexcept { return tag == GUMBO_TAG_LAST; }

    bool match(gumbo_element const& e) const noexcept;

    bool parse(std::string_view const& sel) noexcept
    {
        if(sel.empty())
            return false;

        auto i = sel.npos;
        auto j = sel.npos;

        if(sel[0] == '*')
        {
            i = 1;
            tag = GUMBO_TAG_LAST;
        }
        else
        {
            i = sel.find_first_of(".# ");

            tag = _g_to_tag(sel.substr(0, i));

            if(tag == GUMBO_TAG_UNKNOWN)
                return false;
        }

        while(i < sel.size())
        {
            std::string_view k, v;

            // find key
            if(sel[i] == '.')
            {
                k = "class";
                ++i;
            }
            else if(sel[i] == '#')
            {
                k = "id";
                ++i;
            }
            else // space
            {
                ++i; // skip space
                j = sel.find_first_of("=.# ", i);

                if(i == j)
                    return false;

                k = sel.substr(i, j - i);
                i = j;
            }

            // find value

        #if 1 // handle xml attribute quoted value
            if(char q = sel[i]; q == '"' || q == '\'') // quoted value
            {
                ++i; // skip quote
                j = sel.find(q, i);
                v = sel.substr(i, j - i);
                i = sel.find_first_of(".# ", j);
            }
            else
            {
                j = sel.find_first_of(".# ", i);
                v = sel.substr(i, j - i);
                i = j;
            }

        #else
            j = sel.find_first_of(".# ", i);
            v = sel.substr(i, j - i);
            i = j;
        #endif

            attrs.emplace_back(k, v);
        }

        return true;
    }
};


template<class F>
struct gumbo_combo_sel
{
    DSK_NO_UNIQUE_ADDR F f;

    constexpr bool match(gumbo_element const& e) const noexcept
    {
        return f(e);
    }
};

template<class F>
gumbo_combo_sel(F) -> gumbo_combo_sel<F>;


                  void _gumbo_sel_impl(gumbo_tag_sel&);
                  void _gumbo_sel_impl(gumbo_simple_sel&);
template<class F> void _gumbo_sel_impl(gumbo_combo_sel<F>&);
template<class T> concept _gumbo_sel_ = requires(std::remove_cvref_t<T>& t){ dsk::_gumbo_sel_impl(t); };


constexpr auto&& to_gumbo_sel(_gumbo_sel_ auto&&      s) noexcept { return DSK_FORWARD(s); }
constexpr auto   to_gumbo_sel(_char_str_ auto const& s)          { return gumbo_simple_sel{s}; }
constexpr auto   to_gumbo_sel(GumboTag               t) noexcept { return gumbo_tag_sel{t}; }


template<class L, class R>
concept _two_gumbo_sel_or_at_most_one_str_or_tag_ = (_gumbo_sel_<L> && _gumbo_sel_<R>)
                                                    || (_gumbo_sel_<L> && (_char_str_<R> || _no_cvref_same_as_<R, GumboTag>))
                                                    || ((_char_str_<L> || _no_cvref_same_as_<L, GumboTag>) && _gumbo_sel_<R>);

template<class L, class R>
constexpr auto operator&&(L&& l, R&& r) requires(_two_gumbo_sel_or_at_most_one_str_or_tag_<L, R>)
{
    return gumbo_combo_sel
    (
        [l = to_gumbo_sel(DSK_FORWARD(l)), r = to_gumbo_sel(DSK_FORWARD(r))](gumbo_element const& e)
        {
            return l.match(e) && r.match(e);
        }
    );
}

template<class L, class R>
constexpr auto operator||(L&& l, R&& r) requires(_two_gumbo_sel_or_at_most_one_str_or_tag_<L, R>)
{
    return gumbo_combo_sel
    (
        [l = to_gumbo_sel(DSK_FORWARD(l)), r = to_gumbo_sel(DSK_FORWARD(r))](gumbo_element const& e)
        {
            return l.match(e) || r.match(e);
        }
    );
}

// match l but not r, same as (l && !r)
template<class L, class R>
constexpr auto operator-(L&& l, R&& r) requires(_two_gumbo_sel_or_at_most_one_str_or_tag_<L, R>)
{
    return gumbo_combo_sel
    (
        [l = to_gumbo_sel(DSK_FORWARD(l)), r = to_gumbo_sel(DSK_FORWARD(r))](gumbo_element const& e)
        {
            return l.match(e) && !r.match(e);
        }
    );
}

constexpr auto operator!(_gumbo_sel_ auto&& s)
{
    return gumbo_combo_sel
    (
        [s = DSK_FORWARD(s)](gumbo_element const& e)
        {
            return ! s.match(e);
        }
    );
}



template<class T>
class gumbo_vector
{
    static_assert(sizeof(T) == sizeof(void*)); // T must only have one pointer data member, and no pad.

    T*       _d = nullptr;
    unsigned _n = 0;

public:
    gumbo_vector() = default;
    gumbo_vector(GumboVector const& v) noexcept : _d{reinterpret_cast<T*>(v.data)}, _n{v.length} {}

    unsigned size () const noexcept { return _n             ; }
    T const* data () const noexcept { return _d             ; }
    T const* begin() const noexcept { return data()         ; }
    T const* end  () const noexcept { return data() + size(); }

    T const& operator[](unsigned i) noexcept { DSK_ASSERT(i < size()); return *(data() + i); }
};


class gumbo_attr
{
    friend class gumbo_element;

    GumboAttribute* _h = nullptr;

    gumbo_attr(GumboAttribute* p) noexcept : _h(p) {}

public:
    gumbo_attr() = default;

    explicit operator bool() const noexcept { return _h != nullptr; }

    GumboAttributeNamespaceEnum ns() const noexcept { return _h->attr_namespace; }

    std::string_view name () const noexcept { return _h->name ; }
    std::string_view value() const noexcept { return _h->value; }
    std::string_view ori_name () const noexcept { return _g_to_strv(_h->original_name ); }
    std::string_view ori_value() const noexcept { return _g_to_strv(_h->original_value); }

    GumboSourcePosition const& name_start () const noexcept { return _h->name_start ; }
    GumboSourcePosition const& name_end   () const noexcept { return _h->name_end   ; }
    GumboSourcePosition const& value_start() const noexcept { return _h->value_start; }
    GumboSourcePosition const& value_end  () const noexcept { return _h->value_end  ; }
};


enum gumbo_search_type
{
    gs_self,              // search target self.
    gs_direct_child,      // search all the target's direct child.
    gs_until_found,       // search target's descendants. if a node matches, its descendants will be skipped.
    gs_until_found_first, // search target's descendants until first matched node was found.
    gs_all                // search all the target's descendants.
};


// bind a literal with search type, mainly used by operator*() for united text.
template<gumbo_search_type Type>
struct gumbo_slit : std::string_view
{
    static constexpr gumbo_search_type type = Type;
    using std::string_view::std::string_view;
};

template<gumbo_search_type Default>
constexpr gumbo_slit<Default> to_gumbo_slit(_char_str_ auto const& s) noexcept
{
    return {s};
}

template<gumbo_search_type, gumbo_search_type Type>
constexpr gumbo_slit<Type> const& to_gumbo_slit(gumbo_slit<Type> const& s) noexcept
{
    return s;
}


inline namespace gumbo_literals{

constexpr gumbo_slit<gs_self> operator""_self(char const* s, size_t n) noexcept
{
    return {s, n};
}

constexpr gumbo_slit<gs_direct_child> operator""_child(char const* s, size_t n) noexcept
{
    return {s, n};
}

constexpr gumbo_slit<gs_all> operator""_all(char const* s, size_t n) noexcept
{
    return {s, n};
}

constexpr gumbo_slit<gs_until_found_first> operator""_first(char const* s, size_t n) noexcept
{
    return {s, n};
}

} // namespace gumbo_literals



template<class D>
class gumbo_searchable
{
    D const* derived() const { return static_cast<D const*>(this); }

public:
    template<gumbo_search_type ST = gs_until_found> // since text node doesn't have child this is same as gs_all
    vector<gumbo_text> text_nodes() const;

    template<gumbo_search_type ST = gs_until_found>
    string united_text(std::string_view const& defAndSep) const;

    template<gumbo_search_type ST>
    gumbo_elm_coll elms(auto const& sel) const;


    // % / * operators have same precedence, so they can be chained without parentheses;
    // their precedence are also higher than << and >>, so can be used in stream operator with out parentheses.

    auto operator%(auto const& sel) const { return elms<gs_until_found>(sel); }
    auto operator/(auto const& sel) const { return elms<gs_direct_child>(sel); }

    // c * "<>\n" : gs_until_found/gs_all text nodes, combinded with each end with '\n'
    // c * "<>\n"_child : gs_direct_child
    string operator*(auto const& defAndSep) const
    {
        decltype(auto) sl = to_gumbo_slit<gs_until_found>(defAndSep);
        return united_text<sl.type>(sl);
    }
};


class gumbo_node : public gumbo_searchable<gumbo_node>
{
    friend class gumbo_output;
    friend class gumbo_document;
    friend class gumbo_element;
    friend class gumbo_text;
    friend class gumbo_elm_coll;

    GumboNode* _h = nullptr;

    gumbo_node(GumboNode* p) noexcept : _h(p) {}

public:
    gumbo_node() = default;

    explicit operator bool() const noexcept { return _h != nullptr; }

    GumboNodeType   type         () const noexcept { return _h->type               ; }
    GumboParseFlags parse_flags  () const noexcept { return _h->parse_flags        ; }
    gumbo_node      parent       () const noexcept { return _h->parent             ; }
    size_t          idx_in_parent() const noexcept { return _h->index_within_parent; }

    gumbo_vector<gumbo_node> children() const
    {
        switch(type())
        {
            case GUMBO_NODE_DOCUMENT: return _h->v.document.children;
            case GUMBO_NODE_ELEMENT : return _h->v.element.children ;
        }

        return {};
    }

    template<gumbo_search_type ST>
    void foreach_if(auto&& f) const
    {
        if constexpr(ST == gs_self)
        {
            f(*this);
        }
        else
        {
            for(auto const& c : children())
            {
                if constexpr(ST == gs_direct_child)
                {
                    DSK_FORWARD(f)(c);
                }
                else if constexpr(ST == gs_until_found)
                {
                    if(! f(c))
                        c.foreach_if<ST>(DSK_FORWARD(f));
                }
                else if constexpr(ST == gs_until_found_first)
                {
                    if(f(c))
                        return;
                    c.foreach_if<ST>(DSK_FORWARD(f));
                }
                else if constexpr(ST == gs_all)
                {
                    f(c);
                    c.foreach_if<ST>(DSK_FORWARD(f));
                }
                else
                {
                    static_assert(false, "unsupported gumbo_serach_type");
                }
            }
        }
    }
};


class gumbo_document : public gumbo_node
{
    GumboDocument const& priv() const noexcept { return _h->v.document; }

public:
    gumbo_document(gumbo_node const& n) noexcept
    {
        if(n && n.type() == GUMBO_NODE_DOCUMENT)
            _h = n._h;
    }

    std::string_view name  () const noexcept { return priv().name             ; }
    std::string_view pub_id() const noexcept { return priv().public_identifier; }
    std::string_view sys_id() const noexcept { return priv().system_identifier; }

    bool                has_doctype() const noexcept { return priv().has_doctype         ; }
    GumboQuirksModeEnum quirks_mode() const noexcept { return priv().doc_type_quirks_mode; }
};


class gumbo_element : public gumbo_node
{
    GumboElement const& priv() const noexcept { return _h->v.element; }

public:
    gumbo_element() = default;

    gumbo_element(gumbo_node const& n) noexcept
    {
        if(n && n.type() == GUMBO_NODE_ELEMENT)
            _h = n._h;
    }

    GumboTag           tag   () const noexcept { return priv().tag;           }
    GumboNamespaceEnum tag_ns() const noexcept { return priv().tag_namespace; }

    std::string_view ori_tag    () const noexcept { return _g_to_strv(priv().original_tag    ); }
    std::string_view ori_end_tag() const noexcept { return _g_to_strv(priv().original_end_tag); }

    GumboSourcePosition const& start_pos() const noexcept { return priv().start_pos; }
    GumboSourcePosition const& end_pos  () const noexcept { return priv().end_pos;   }

    gumbo_vector<gumbo_attr> attrs() const { return priv().attributes; }

    gumbo_attr get_attr(std::string_view const& name) const noexcept
    {
        for(gumbo_attr a : attrs())
        {
            if(a.name() == name)
                return a;
        }
        return {};
    }

    std::string_view attrv(std::string_view const& name, std::string_view const& default_ = std::string_view{}) const noexcept
    {
        if(auto a = get_attr(name))
            return a.value();
        return default_;
    }

    bool has_attr(std::string_view const& k) const noexcept
    {
        return !!get_attr(k);
    }

    bool has_attr(std::string_view const& k, std::string_view const& v) const noexcept
    {
        if(auto a = get_attr(k))
            return a.value() == v;
        return false;
    }
};


class gumbo_text : public gumbo_node
{
    GumboText const& priv() const noexcept { return _h->v.text; }

public:
    gumbo_text(gumbo_node const& n) noexcept
    {
        if(n && n.type() == GUMBO_NODE_TEXT)
            _h = n._h;
    }

    std::string_view text    () const noexcept { return priv().text                     ; }
    std::string_view ori_text() const noexcept { return _g_to_strv(priv().original_text); }

    GumboSourcePosition const& start_pos() const noexcept { return priv().start_pos; }
};


class gumbo_elm_coll : public vector<gumbo_element>, public gumbo_searchable<gumbo_elm_coll>
{
public:
    void append(gumbo_elm_coll const& c)
    {
        insert(end(), c.begin(), c.end());
    }

    template<gumbo_search_type ST>
    void foreach_if(auto&& f) const
    {
        for(auto const& e : *this)
            e.foreach_if<ST>(DSK_FORWARD(f));
    }

    explicit operator bool() const noexcept { return size(); }

    // get attr value of first element if any.
    // attr: "attrName[=defVal]", defval default to ""
    // - has higher precedence than << and >>, so can be used in stream operator with out parentheses.
    std::string_view operator-(std::string_view const& attr) const noexcept
    {
        auto p = attr.find('=');
        auto name   = attr.substr(0, p);
        auto defVal = (p != npos && p + 1 < attr.size()) ? attr.substr(p + 1) : "";

        return size() ? (*this)[0].attrv(name, defVal) : defVal;
    }
};


bool gumbo_simple_sel::match(gumbo_element const& e) const noexcept
{
    DSK_ASSERT(e);

    if(! match_any_tag())
    {
        if(tag != e.tag())
            return false;
    }

    for(auto const& [k, v] : attrs)
    {
        if(auto a = e.get_attr(k))
        {
            if(v.size() && v != a.value())
                return false;
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool gumbo_tag_sel::match(gumbo_element const& e) const noexcept
{
    return tag == e.tag();
}


template<class D>
template<gumbo_search_type ST>
gumbo_elm_coll gumbo_searchable<D>::elms(auto const& sel) const
{
    gumbo_elm_coll v;

    decltype(auto) hsel = to_gumbo_sel(sel);

    derived()->template foreach_if<ST>(
        [&v, &hsel](gumbo_element e) mutable
        {
            if(e && hsel.match(e))
            {
                v.emplace_back(e);
                return true;
            }
            return false;
        }
    );

    return v;
}

template<class D>
template<gumbo_search_type ST>
vector<gumbo_text> gumbo_searchable<D>::text_nodes() const
{
    vector<gumbo_text> v;

    derived()->foreach_if<ST>(
        [&v](gumbo_text t) mutable
        {
            if(t)
            {
                v.emplace_back(t);
                return true;
            }
            return false;
        }
    );

    return v;
}

// "def<>sep"
// def will return when nothing found, optional and default to ""
// sep will be appended to each text, optional and default to " "
template<class D>
template<gumbo_search_type ST>
string gumbo_searchable<D>::united_text(std::string_view const& defAndSep) const
{
    auto p = defAndSep.rfind("<>");

    auto def = defAndSep.substr(0, p);
    auto sep = (p != npos && p + 2 < defAndSep.size()) ? defAndSep.substr(p + 2) : " ";

    string ut;

    derived()->foreach_if<ST>(
        [&sep, &ut](gumbo_text t) mutable
        {
            if(t)
            {
                ut += t.text();
                ut += sep;
                return true;
            }
            return false;
        }
    );

    if(ut.size())
        ut.resize(ut.size() - sep.size());
    else
        ut = def;

    return ut;
}


// GumboError isn't in public api yet.
// class gumbo_error
// {
//     GumboError* _h;
//
// public:
//     gumbo_error(GumboError* p) : _h(p) {}
//
//     GumboErrorType const&      type    () const { return _h->type         ; }
//     GumboSourcePosition const& pos     () const { return _h->position     ; }
//     std::string_view           ori_text() const { return _h->original_text; }
//
//     // pick one error according to type()
//     uint64_t                        codepoint() const { return _h->v.codepoint       ; }
//     std::string_view                text     () const { return _g_to_strv(_h->v.text); }
//     GumboTokenizerError      const& tokenizer() const { return _h->v.tokenizer       ; }
//     GumboDuplicateAttrError  const& dup_attr () const { return _h->v.duplicate_attr  ; }
//     GumboInternalParserError const& parser   () const { return _h->v.parser          ; }
// };


// Gumbo objects are all held by gumbo_output
class gumbo_output
{
    unique_handle<
        GumboOutput*,
        [](GumboOutput* p) { gumbo_destroy_output(&kGumboDefaultOptions, p); }
    > _h;

public:
    gumbo_output() = default;
    gumbo_output(GumboOutput* p) : _h{p} {}

    explicit operator bool() const noexcept { return _h.valid(); }

    gumbo_document document() const { return gumbo_node(_h->document); } //
    gumbo_element  root    () const { return gumbo_node(_h->root    ); } // <html>...</html>

    bool has_err() const { return _h->errors.length > 0; }
//     gumbo_vector<gumbo_error> errors() const { return {_h->errors}; }

    template<class T>
    using gumbo_expected = expected<T, gumbo_errc>;

    static gumbo_expected<gumbo_output> parse_utf8(_char_str_ auto const& s, GumboOptions const& opts = kGumboDefaultOptions) noexcept
    {
        if(GumboOutput* o = gumbo_parse_with_options(&opts, str_data(s), str_size(s)))
        {
            return o;
        }

        return gumbo_errc::parse_failed;
    }

    // encoding detecting order: BOM => http header(Content-Type: charset=xxx) => html/head/<meta charset=xxx>.
    // ref: https://www.w3.org/International/questions/qa-html-encoding-declarations#answer
    // if all detection fails, we will just assume srcCs == dstCs.
    // you should set 'srcCs' to the specified http header if any or empty(so it would try finding <meta charset=xxx> if bom detection failed).
    // with SkipCharsetDetection == true, you can force it to use the specified charsets.
    // NOTE: destCp should compatible with ascii, otherwise parse may fail.
    template<
        class CsCvt
        #if __has_include(<unicode/ucnv.h>)
            = icu_charset_cvt
        #endif
        ,
        bool SkipCsDetection = false
    >
    static gumbo_expected<gumbo_output> parse(std::string_view const& s,
                                              std::string_view srcCs = "", std::string_view dstCs = "UTF-8",
                                              GumboOptions const& opts = kGumboDefaultOptions) noexcept
    {
        DSK_ASSERT(dstCs.size());

        gumbo_output o;

        if constexpr(! SkipCsDetection)
        {
            if(auto w = detect_bom(s))
            {
                srcCs = w.charset;
            }
            else if(srcCs.empty())
            {
                DSK_E_TRY(o, parse_utf8(s, opts));
                srcCs = ascii_trimed_view(o.root() / "head" / "meta" - "charset");
            }
        }

        if(srcCs.empty() || srcCs == dstCs)
        {
            if(o) return o;
            else  return parse_utf8(s, opts);
        }

        DSK_E_TRY_FWD(utf8s, CsCvt::to_dst(srcCs, s, dstCs));
        return parse_utf8(utf8s, opts);
    }
};


} // namespace dsk
