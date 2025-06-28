cmake_minimum_required(VERSION 3.31)

project(dskTests CXX)

include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(dsk REQUIRED)
  enable_testing()
endif()

find_package(doctest REQUIRED)
include(../cmake/doctest.cmake)


# ---- Tests ----


# test_basic

add_executable(test_basic basic/main.cpp)

target_link_libraries(test_basic PRIVATE dsk::asio dsk::tbb doctest::doctest)

doctest_discover_tests(test_basic)


# test_asio

add_executable(test_asio asio/main.cpp)

target_link_libraries(test_asio PRIVATE dsk::asio doctest::doctest)

doctest_discover_tests(test_asio)


# test_http

add_executable(test_http http/main.cpp)

target_link_libraries(test_http PRIVATE dsk::http doctest::doctest)

doctest_discover_tests(test_http)


# test_curl

add_executable(test_curl curl/main.cpp)

target_link_libraries(test_curl PRIVATE dsk::curl doctest::doctest)

doctest_discover_tests(test_curl)


# test_grpc

add_executable(test_grpc grpc/main.cpp)

target_link_libraries(test_grpc PRIVATE dsk::grpc doctest::doctest)

include(../cmake/AsioGrpcProtobufGenerator.cmake)
asio_grpc_protobuf_generate(
    GENERATE_GRPC
    TARGET test_grpc
    OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/example_proto"
    PROTOS "${CMAKE_CURRENT_SOURCE_DIR}/grpc/example.proto")

doctest_discover_tests(test_grpc)


# test_jser

add_executable(test_jser jser/main.cpp)

target_link_libraries(test_jser PRIVATE dsk::jser)

doctest_discover_tests(test_jser)


# test_sqlite

add_executable(test_sqlite sqlite/main.cpp)

target_link_libraries(test_sqlite PRIVATE dsk::sqlite3 dsk::guni doctest::doctest)

doctest_discover_tests(test_sqlite)


# test_mysql

add_executable(test_mysql mysql/main.cpp)

target_link_libraries(test_mysql PRIVATE dsk::mysql doctest::doctest)

doctest_discover_tests(test_mysql)


# test_pq

add_executable(test_pq pq/main.cpp)

target_link_libraries(test_pq PRIVATE dsk::pq doctest::doctest)

doctest_discover_tests(test_pq)


# test_redis

add_executable(test_redis redis/main.cpp)

target_link_libraries(test_redis PRIVATE dsk::redis doctest::doctest)

doctest_discover_tests(test_redis)


# test_compr

add_executable(test_compr compr/main.cpp)

target_link_libraries(test_compr PRIVATE dsk::compr)

doctest_discover_tests(test_compr)


# ---- End-of-file commands ----

add_folders(Test)
