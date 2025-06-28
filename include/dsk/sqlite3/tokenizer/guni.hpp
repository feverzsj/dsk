#pragma once

#include <dsk/util/handle.hpp>
#include <dsk/util/vector.hpp>
#include <dsk/default_allocator.hpp>
#include <dsk/sqlite3/tokenizer/fts5.hpp>
#include <dsk/sqlite3/tokenizer/snowball_stemmer.hpp>

#include <uni_algo/case.h>
#include <uni_algo/norm.h>
#include <uni_algo/ranges_word.h>


namespace dsk{


// A generalized unicode tokenizer:
// 
// Find the Word boundaries via UAX#29.
// If stemmer is specified, stem only letter word.
// Non-letter word will typically be a single character.
//
// NOTE: underscore'_'  and digits are not word boundaries, i.e. they'll be included in word.
//
// Usage:
//      CREATE VIRTUAL TABLE ... USING fts5(..., tokenize='guni [stemmer <language>]');
// 
//      Use to_match_exp() to transform search expression to proper form that matches the tokenization.
//      
template<class Alloc = DSK_DEFAULT_ALLOCATOR<void>>
class guni_fts5_tokenizer_t : public fts5_tokenizer_base<guni_fts5_tokenizer_t<Alloc>, Alloc>
{
    snowball_stemmer _stemmer;

public:
    guni_fts5_tokenizer_t(char const** azArg, int nArg)
    {
        if(nArg == 2 && strcmp(azArg[0], "stemmer") == 0)
        {
            _stemmer = snowball_stemmer(azArg[1]);
            
            if(! _stemmer)
            {
                throw std::runtime_error("guni_fts5_tokenizer: invalid stemmer");
            }

            return;
        }

        if(nArg != 0)
        {
            throw std::runtime_error("guni_fts5_tokenizer: invalid arg.");
        }
    }

    static constexpr char const* name() noexcept { return "guni"; }

    int do_tokenize(int /*flags*/, char const* pText, int nText, char const* /*pLocale*/, int /*nLocale*/, auto&& onToken)
    {
        int err = SQLITE_OK;

        auto view = una::views::word::utf8(std::string_view(pText, nText));

        for(auto it = view.begin(); it != view.end(); ++it)
        {
            if(! it.is_word())
            {
                continue;
            }

            auto word = *it;
            auto begIdx = word.data() - pText;
            auto endIdx = begIdx + word.size();

            if(it.is_word_letter())
            {
                auto lower = una::cases::to_lowercase_utf8(word, rebind_alloc<Alloc, char>());

                if(_stemmer)
                {
                    auto nfc = una::norm::to_nfc_utf8(std::string_view(lower), rebind_alloc<Alloc, char>());
                    auto stemmed = _stemmer.stem(nfc);

                    err = onToken(stemmed, begIdx, endIdx);
                }
                else
                {
                    err = onToken(lower, begIdx, endIdx);
                }
            }
            else
            {
                err = onToken(word, begIdx, endIdx);
            }

            if(err)
            {
                break;
            }
        }

        if(err == SQLITE_DONE)
        {
            err = SQLITE_OK;
        }

        return err;
    }

    // Transform search expression to proper form that matches the tokenization:
    // 
    //      '(hello OR Grüß) NOT 你好' -> '(hello OR Grüß) NOT NEAR(你 好 , 0)'
    //      
    // NOTE: NEAR not allowed in 's'
    template<_resizable_byte_buf_ B = string>
    static B to_match_exp(_byte_str_ auto&& s)
    {
        using ch_t = buf_val_t<B>;

        B b;

        auto q = str_view<ch_t>(s);

        vector<decltype(q), Alloc> nears;

        auto appendNears = [&]()
        {
            if(nears.size() == 1)
            {
                append_buf(b, nears[0]);
            }
            else if(nears.size() > 1)
            {
                append_buf(b, str_view<ch_t>("NEAR("));

                for(auto& word : nears)
                {
                    append_buf(b, word, str_view<ch_t>(" "));
                }

                append_buf(b, str_view<ch_t>(cat_as_str(", ", nears.size() - 2, ")")));
            }

            nears.clear();
        };

        auto view = una::views::word::utf8(q);

        for(auto it = view.begin(); it != view.end(); ++it)
        {
            auto seg = *it;

            if(it.is_word() //&& ! it.is_word_letter()
               &&(nears.empty() || buf_end(nears.back()) == buf_begin(seg)))
            {
                nears.emplace_back(seg);
            }
            else
            {
                appendNears();
                append_buf(b, seg);
            }
        }

        appendNears();

        return b;
    }
};


using guni_fts5_tokenizer = guni_fts5_tokenizer_t<>;


} // namespace std