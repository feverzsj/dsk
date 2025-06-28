#pragma once

#include <dsk/config.hpp>
#include <cassert>
#include <cstdio>


#ifndef DSK_REPORTF
    #define DSK_REPORTF(...)                   \
        do{                                    \
            std::fprintf(stderr, __VA_ARGS__); \
            std::fflush(stderr);               \
        } while(false)                         \
    /**/
#endif


#ifndef DSK_DUMP_STACK

    #ifdef __cpp_lib_stacktrace
        #include <stacktrace>
        #define DSK_DUMP_STACK() DSK_REPORTF("%s\n", std::to_string(std::stacktrace::current()).data());
    #else
        #define DSK_DUMP_STACK()
    #endif

#endif


#ifndef DSK_DUMP_LINE_FORMAT

    #ifdef _WIN32
        // This style lets Visual Studio follow errors back to the source file.
        #define DSK_DUMP_LINE_FORMAT "%s(%d)"
    #else
        #define DSK_DUMP_LINE_FORMAT "%s:%d"
    #endif

#endif

#ifndef DSK_ABORT

    #define DSK_ABORT(msg, ...)                                              \
        do{                                                                  \
            DSK_REPORTF(DSK_DUMP_LINE_FORMAT ": fatal error: \"" msg "\"\n", \
                        __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__);       \
            DSK_DUMP_STACK();                                                \
            DSK_TRAP();                                                      \
        }while(false)                                                        \
    /**/

#endif


#define DSK_RELEASE_ASSERT(...)                                                   \
    static_cast<void>(                                                            \
        DSK_LIKELY(__VA_ARGS__) ? static_cast<void>(0)                            \
                                : [&]{ DSK_ABORT("assert(%s)", #__VA_ARGS__); }() \
    )                                                                             \
/**/

#define DSK_RELEASE_ASSERTF(cond, fmt, ...)                                                          \
    static_cast<void>(                                                                               \
        DSK_LIKELY(cond) ? static_cast<void>(0)                                                      \
                         : [&]{ DSK_ABORT("assertf(%s): " fmt, #cond __VA_OPT__(,) __VA_ARGS__); }() \
    )                                                                                                \
/**/


#ifdef NDEBUG
    #define DSK_DEBUGF(...) static_cast<void>(0)
    #define DSK_ASSERT(...) static_cast<void>(0)
    #define DSK_ASSERTF(cond, fmt, ...) static_cast<void>(0)
    #define DSK_VERIFY(...) if(__VA_ARGS__){}do{}while(false)
#else
    #define DSK_DEBUGF(...) DSK_REPORTF(__VA_ARGS__)
    #define DSK_ASSERT(...) DSK_RELEASE_ASSERT(__VA_ARGS__)
    #define DSK_ASSERTF(cond, fmt, ...) DSK_RELEASE_ASSERTF(cond, fmt, __VA_ARGS__)
    #define DSK_VERIFY(...) DSK_ASSERT(__VA_ARGS__)
#endif
