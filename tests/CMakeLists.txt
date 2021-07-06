cmake_minimum_required(VERSION 3.1)

find_package(Catch2 REQUIRED)

add_executable(test basic.cpp)
target_link_libraries(test PRIVATE Catch2::Catch2 terraqtt::terraqtt)

include(Catch)
catch_discover_tests(test)
