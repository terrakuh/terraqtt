cmake_minimum_required(VERSION 3.1)

project(mqtt)

set(CMAKE_CXX_STANDARD 11)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

add_executable(main "main.cpp")
target_link_libraries(main Boost::boost Boost::system Threads::Threads)
