cmake_minimum_required(VERSION 2.8.7)

project(cpp-argparsy CXX)

include_directories(${CMAKE_CURRENT_LIST_DIR})
add_library(cpp-argparsy program_options.cpp)

# ------------------------------------------------------------------------------

include(CTest)
enable_testing()

if(BUILD_TESTING)
  add_executable(main_test ${CMAKE_CURRENT_LIST_DIR}/test/main.cpp)
  target_link_libraries(main_test cpp-argparsy)

  add_test(
    NAME show_help
    COMMAND main_test -h)
  add_test(
    NAME try_args
    COMMAND main_test 1 one.cpp)
  add_test(
    NAME try_all_args
    COMMAND main_test -O -b -u 42 -v 58 58 58 2 first.cpp second.cpp)
endif(BUILD_TESTING)

