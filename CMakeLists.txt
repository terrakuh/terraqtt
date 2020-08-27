cmake_minimum_required(VERSION 3.1)

project(mqtt)

set(CMAKE_CXX_STANDARD 11)

option(MQTT_BUILD_EXAMPLES ON)

find_package(Boost 1.65 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

if(MQTT_BUILD_EXAMPLES)
  add_executable(playground "examples/playground.cpp")
  target_link_libraries(main Boost::boost Boost::system Threads::Threads)
endif()

install(DIRECTORY mqtt
  DESTINATION include)
