#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <dsk/util/string.hpp>
#include <dsk/compr/bz2.hpp>
#include <dsk/compr/zlib.hpp>
#include <dsk/compr/zstd.hpp>


using namespace dsk;

inline constexpr std::string_view in = R"(
The monologue is near the conclusion of Blade Runner, in which detective Rick Deckard (played by Harrison Ford)
has been ordered to track down and kill Roy Batty, a rogue artificial "replicant". During a rooftop chase in
heavy rain, Deckard misses a jump and hangs on to the edge of a building by his fingers, about to fall to his
death. Batty turns back and lectures Deckard briefly about how the tables have turned, but pulls him up to
safety at the last instant. Recognizing that his limited lifespan is about to end, Batty addresses his
shocked nemesis, reflecting on his own experiences and mortality, with dramatic pauses between each statement:
In the documentary Dangerous Days: Making Blade Runner, Hauer, director Ridley Scott, and screenwriter David
Peoples confirm that Hauer significantly modified the speech. In his autobiography, Hauer said he merely cut
the original scripted speech by several lines, adding only, "All those moments will be lost in time, like
tears in rain". One earlier version in Peoples' draft screenplays was:
I've known adventures, seen places you people will never see, I've been Offworld and back... frontiers!
I've stood on the back deck of a blinker bound for the Plutition Camps with sweat in my eyes watching
stars fight on the shoulder of Orion... I've felt wind in my hair, riding test boats off the black galaxies
and seen an attack fleet burn like a match and disappear. I've seen it, felt it...!
)";

void do_test(auto&& comp, auto&& decomp)
{
    string compr;
    auto rCompr = comp(compr, in);
    CHECK(! has_err(rCompr));

    string decompr;
    auto rDecompr = decomp(decompr, compr);
    CHECK(! has_err(rDecompr));
    CHECK(get_val(rDecompr).nIn == buf_size(compr));
    CHECK(get_val(rDecompr).nOut == buf_size(decompr));
    CHECK(get_val(rDecompr).isEnd);

    CHECK(decompr == in);

}

TEST_CASE("compr")
{
    SUBCASE("bz2")
    {
        do_test([](auto& d, auto&& in){ return bz2_compress(d, in); },
                [](auto& d, auto&& in){ return bz2_decompress(d, in); });

        do_test([](auto& d, auto&& in){ return bz2_compress(d, in, {}, in.size()/3 + 1); },
                [](auto& d, auto&& in){ return bz2_decompress(d, in, {}, in.size()/3 + 1); });
    }// SUBCASE("bz2")

    SUBCASE("zlib")
    {
        do_test([](auto& d, auto&& in){ return zlib_compress(d, in); },
                [](auto& d, auto&& in){ return zlib_decompress(d, in); });

        do_test([](auto& d, auto&& in){ return zlib_compress(d, in, {}, in.size()/3 + 1); },
                [](auto& d, auto&& in){ return zlib_decompress(d, in, {}, in.size()/3 + 1); });
    }// SUBCASE("zlib")

    SUBCASE("gzip auto detect")
    {
        do_test([](auto& d, auto&& in){ return zlib_compress(d, in, {.gzip = true}); },
                [](auto& d, auto&& in){ return zlib_decompress(d, in); });

        do_test([](auto& d, auto&& in){ return zlib_compress(d, in, {.gzip = true}, in.size()/3 + 1); },
                [](auto& d, auto&& in){ return zlib_decompress(d, in, {}, in.size()/3 + 1); });
    }// SUBCASE("gzip auto detect")

    SUBCASE("gzip forced")
    {
        do_test([](auto& d, auto&& in){ return zlib_compress(d, in, {.gzip = true}); },
                [](auto& d, auto&& in){ return zlib_decompress(d, in, {.gzip = true}); });

        do_test([](auto& d, auto&& in){ return zlib_compress(d, in, {.gzip = true}, in.size()/3 + 1); },
                [](auto& d, auto&& in){ return zlib_decompress(d, in, {.gzip = true}, in.size()/3 + 1); });
    }// SUBCASE("gzip forced")

    SUBCASE("zstd")
    {
        do_test([](auto& d, auto&& in){ return zstd_compress(d, in); },
                [](auto& d, auto&& in){ return zstd_decompress(d, in); });

        do_test([](auto& d, auto&& in)
                {
                    zstd_compressor compr;
                    return compr.append<ZSTD_e_end>(d, in, in.size()/3 + 1);
                },
                [](auto& d, auto&& in){ return zstd_decompress(d, in, in.size()/3 + 1); });
    }// SUBCASE("zstd")

} // TEST_CASE("compr")
