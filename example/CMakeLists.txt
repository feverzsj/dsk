cmake_minimum_required(VERSION 3.31)

project(dskExamples CXX)

include(../cmake/folders.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(dsk REQUIRED)
endif()


function(add_run_example NAME)
  add_custom_target("run_${NAME}" COMMAND "${NAME}" VERBATIM)
  add_dependencies("run_${NAME}" "${NAME}")
endfunction()


# example_http_static_server

add_executable(example_http_static_server http_static_server/main.cpp)

target_link_libraries(example_http_static_server PRIVATE dsk::http)

add_run_example(example_http_static_server)


# example_compr_conv

add_executable(example_compr_conv compr_conv/main.cpp)

target_link_libraries(example_compr_conv PRIVATE dsk::asio dsk::compr)

add_run_example(example_compr_conv)


# example_wiki_hist

add_executable(example_wiki_hist wiki_hist/main.cpp)

find_package(Backward REQUIRED)
target_link_libraries(example_wiki_hist PRIVATE dsk::asio dsk::compr dsk::jser dsk::sqlite3 Backward::Interface)

add_run_example(example_wiki_hist)


add_folders(Example)
