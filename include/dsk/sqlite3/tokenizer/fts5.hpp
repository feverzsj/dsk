#pragma once

#include <dsk/sqlite3/conn.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/allocator.hpp>
#include <dsk/util/allocate_unique.hpp>


namespace dsk{


template<class Derived, class Alloc>
class fts5_tokenizer_base
{
public:
    using allocator_type = rebind_alloc<Alloc, Derived>;

protected:
    static Derived* do_create(char const** azArg, int nArg)
    {
        return allocate_unique<Derived>(allocator_type(), azArg, nArg).release();
    }

    static void do_delete(Derived* d)
    {
        allocated_unique_ptr<Derived, allocator_type>{d};
    }

    static int xCreate(void*, char const** azArg, int nArg, Fts5Tokenizer** ppOut) noexcept
    {
        try
        {
            if(auto* d = Derived::do_create(azArg, nArg))
            {
                *ppOut = reinterpret_cast<Fts5Tokenizer*>(d);
                return SQLITE_OK;
            }
        }
        catch(std::exception const& e)
        {
            DSK_REPORTF("%s_fts5_tokenizer.do_create() throw: %s", Derived::name(), e.what());
        }

        return SQLITE_ERROR;
    }

    static void xDelete(Fts5Tokenizer* d) noexcept
    {
        try
        {
            Derived::do_delete(reinterpret_cast<Derived*>(d));
        }
        catch(std::exception const& e)
        {
            DSK_REPORTF("%s_fts5_tokenizer.do_delete() throw: %s", Derived::name(), e.what());
        }
    }

    static int xTokenize(Fts5Tokenizer* d, void* pCtx, int flags,
                         char const* pText, int nText,
                         char const* pLocale, int nLocale,
                         int (*xToken)(void*, int, char const*, int, int, int)) noexcept
    {
        try
        {
            return reinterpret_cast<Derived*>(d)->do_tokenize
            (
                flags, pText, nText, pLocale, nLocale,
                [pCtx, flags, xToken](_byte_str_ auto const& token, std::integral auto start, std::integral auto end)
                {
                    return xToken(pCtx, flags,
                                  str_data<char>(token), str_size<int>(token),
                                  static_cast<int>(start), static_cast<int>(end));
                }
            );
        }
        catch(std::exception const& e)
        {
            DSK_REPORTF("%s_fts5_tokenizer.do_tokenize() throw: %s", Derived::name(), e.what());
        }

        return SQLITE_ERROR;
    }

public:
    static error_code register_on(sqlite3_conn& c)
    {
        DSK_E_TRY(fts5_api* api, c.fts5());
        fts5_tokenizer_v2 t{2, xCreate, xDelete, xTokenize};
        return static_cast<sqlite3_errc>(api->xCreateTokenizer_v2(api, Derived::name(), api, &t, nullptr));
    }
};


} // namespace std