cmake_minimum_required(VERSION 3.31)

include(cmake/prelude.cmake)

project(
    dsk
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/variables.cmake)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake") # for Findxxx.cmake


# dsk_basic

add_library(dsk_basic INTERFACE)
add_library(dsk::basic ALIAS dsk_basic)

target_include_directories(
    dsk_basic ${warning_guard}
    INTERFACE
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

# better specify std flags manually in presets
#target_compile_features(dsk_basic INTERFACE cxx_std_26)
set_target_properties(dsk_basic PROPERTIES CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED COMPONENTS system numeric_conversion)
target_link_libraries(dsk_basic INTERFACE Boost::system Boost::numeric_conversion)


# default allocators

set(DEFAULT_ALLOCATOR "" CACHE STRING "default allocator")

if(DEFAULT_ALLOCATOR STREQUAL "mimalloc")

    find_package(mimalloc REQUIRED)
    target_link_libraries(dsk_basic INTERFACE $<IF:$<TARGET_EXISTS:mimalloc-static>, mimalloc-static, mimalloc>)
    target_compile_definitions(dsk_basic INTERFACE DSK_DEFAULT_ALLOCATOR_USE_MI)

elseif(DEFAULT_ALLOCATOR STREQUAL "tbb")

    find_package(TBB REQUIRED)
    target_link_libraries(dsk_basic INTERFACE TBB::tbb TBB::tbbmalloc)
    target_compile_definitions(dsk_basic INTERFACE DSK_DEFAULT_ALLOCATOR_USE_TBB)

elseif(DEFAULT_ALLOCATOR STREQUAL "tbb_cache_aligned")

    find_package(TBB REQUIRED)
    target_link_libraries(dsk_basic INTERFACE TBB::tbb TBB::tbbmalloc)
    target_compile_definitions(dsk_basic INTERFACE DSK_DEFAULT_ALLOCATOR_USE_TBB_CACHE_ALIGNED)

elseif(DEFAULT_ALLOCATOR STREQUAL "tbb_scalable")

    find_package(TBB REQUIRED)
    target_link_libraries(dsk_basic INTERFACE TBB::tbb TBB::tbbmalloc)
    target_compile_definitions(dsk_basic INTERFACE DSK_DEFAULT_ALLOCATOR_USE_TBB_SCALABLE)

elseif(DEFAULT_ALLOCATOR STREQUAL "jemalloc")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JEMALLOC jemalloc)
    pkg_search_module(JEMALLOC REQUIRED jemalloc)

    include_directories(${JEMALLOC_INCLUDE_DIRS})
    target_link_libraries(dsk_basic ${JEMALLOC_LIBRARIES})
    target_compile_definitions(dsk_basic INTERFACE DSK_DEFAULT_ALLOCATOR_USE_JE)

elseif(NOT DEFAULT_ALLOCATOR STREQUAL "")

    message(FATAL_ERROR, "Invalid DEFAULT_ALLOCATOR: ${DEFAULT_ALLOCATOR}")

endif()


# dsk_compr

add_library(dsk_compr INTERFACE)
add_library(dsk::compr ALIAS dsk_compr)
target_link_libraries(dsk_compr INTERFACE dsk_basic)

find_package(BZip2)
if(BZip2_FOUND)
    target_link_libraries(dsk_compr INTERFACE BZip2::BZip2)
endif()

find_package(ZLIB)
if(ZLIB_FOUND)
    target_link_libraries(dsk_compr INTERFACE ZLIB::ZLIB)
endif()

find_package(zstd)
if(zstd_FOUND)
    target_link_libraries(dsk_compr INTERFACE zstd::libzstd)
endif()


# dsk_tbb

add_library(dsk_tbb INTERFACE)
add_library(dsk::tbb ALIAS dsk_tbb)
target_link_libraries(dsk_tbb INTERFACE dsk_basic)

find_package(TBB)
if(TBB_FOUND)
    target_link_libraries(dsk_tbb INTERFACE TBB::tbb)
endif()


# dsk_grpc

add_library(dsk_grpc INTERFACE)
add_library(dsk::grpc ALIAS dsk_grpc)
target_link_libraries(dsk_grpc INTERFACE dsk_basic)

find_package(gRPC)
if(gRPC_FOUND)
    target_link_libraries(dsk_grpc INTERFACE gRPC::gpr gRPC::grpc gRPC::grpc++)
endif()


# dsk_asio

add_library(dsk_asio INTERFACE)
add_library(dsk::asio ALIAS dsk_asio)
target_link_libraries(dsk_asio INTERFACE dsk_basic)

find_package(Boost OPTIONAL_COMPONENTS asio)
if(Boost_asio_FOUND)

    target_link_libraries(dsk_asio INTERFACE Boost::asio)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")

        option(ENABLE_IO_URING "Enable io_uring." "${dsk_DEVELOPER_MODE}")
        if(ENABLE_IO_URING)

            find_package(IoUring REQUIRED)
            target_link_libraries(dsk_asio INTERFACE IoUring::IoUring)

            target_compile_definitions(dsk_asio INTERFACE
                BOOST_ASIO_HAS_IO_URING=1
                BOOST_ASIO_DISABLE_EPOLL=1)

        endif()
    endif()
endif()

find_package(OpenSSL)
if(OpenSSL_FOUND)
    target_link_libraries(dsk_asio INTERFACE OpenSSL::SSL)
endif()


# dsk_http

add_library(dsk_http INTERFACE)
add_library(dsk::http ALIAS dsk_http)
target_link_libraries(dsk_http INTERFACE dsk_asio)

find_package(Boost OPTIONAL_COMPONENTS beast)
if(Boost_beast_FOUND)
    target_link_libraries(dsk_http INTERFACE Boost::beast)
endif()

find_package(Boost OPTIONAL_COMPONENTS url)
if(Boost_url_FOUND)
    target_link_libraries(dsk_http INTERFACE Boost::url)
endif()


# dsk_curl

add_library(dsk_curl INTERFACE)
add_library(dsk::curl ALIAS dsk_curl)
target_link_libraries(dsk_curl INTERFACE dsk_http)

find_package(CURL)
if(CURL_FOUND)
    target_link_libraries(dsk_curl INTERFACE CURL::libcurl)
endif()


# dsk_pq

add_library(dsk_pq INTERFACE)
add_library(dsk::pq ALIAS dsk_pq)
target_link_libraries(dsk_pq INTERFACE dsk_asio)

find_package(PostgreSQL)
if(PostgreSQL_FOUND)
    target_link_libraries(dsk_pq INTERFACE PostgreSQL::PostgreSQL)
endif()


# dsk_mysql

add_library(dsk_mysql INTERFACE)
add_library(dsk::mysql ALIAS dsk_mysql)
target_link_libraries(dsk_mysql INTERFACE dsk_asio)

find_package(Boost OPTIONAL_COMPONENTS mysql)
if(Boost_mysql_FOUND)
    target_link_libraries(dsk_mysql INTERFACE Boost::mysql)
endif()


# dsk_sqlite

add_library(dsk_sqlite3 INTERFACE)
add_library(dsk::sqlite3 ALIAS dsk_sqlite3)
target_link_libraries(dsk_sqlite3 INTERFACE dsk_basic)

find_package(unofficial-sqlite3)
if(unofficial-sqlite3_FOUND)
    target_link_libraries(dsk_sqlite3 INTERFACE unofficial::sqlite3::sqlite3)
endif()


# dsk_guni

add_library(dsk_guni INTERFACE)
add_library(dsk::guni ALIAS dsk_guni)
target_link_libraries(dsk_guni INTERFACE dsk_sqlite3)

find_package(uni-algo)
if(uni-algo_FOUND)
    target_link_libraries(dsk_guni INTERFACE uni-algo::uni-algo)
endif()

find_package(stemmer)
if(stemmer_FOUND)
    target_link_libraries(dsk_guni INTERFACE dsk_sqlite3 stemmer::stemmer)
endif()


# dsk_redis

add_library(dsk_redis INTERFACE)
add_library(dsk::redis ALIAS dsk_redis)
target_link_libraries(dsk_redis INTERFACE dsk_asio)

find_package(Boost OPTIONAL_COMPONENTS redis)
if(Boost_redis_FOUND)
    target_link_libraries(dsk_redis INTERFACE Boost::redis)
endif()


# dsk_jser

add_library(dsk_jser INTERFACE)
add_library(dsk::jser ALIAS dsk_jser)
target_link_libraries(dsk_jser INTERFACE dsk_basic)

find_package(simdjson)
if(simdjson_FOUND)
    target_link_libraries(dsk_jser INTERFACE simdjson::simdjson)
endif()

find_package(simdutf)
if(simdutf_FOUND)
    target_link_libraries(dsk_jser INTERFACE simdutf::simdutf)
endif()


# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
    option(BUILD_EXAMPLES "Build examples." "${dsk_DEVELOPER_MODE}")
    if(BUILD_EXAMPLES)
        add_subdirectory(example)
    endif()

    option(BUILD_MEDIA_MAN "Build media man." "${dsk_DEVELOPER_MODE}")
    if(BUILD_MEDIA_MAN)
        add_subdirectory(media_man)
    endif()
endif()

# ---- Developer mode ----

if(NOT dsk_DEVELOPER_MODE)
    return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
    message(
        AUTHOR_WARNING
        "Developer mode is intended for developers of dsk"
    )
endif()

include(cmake/dev-mode.cmake)
