cmake_minimum_required(VERSION 3.1)

project(terraqtt
	VERSION 0.1.0
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

option(TERRAQTT_BUILD_EXAMPLES ON)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME}
	INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

if(TERRAQTT_BUILD_EXAMPLES)
	find_package(Boost 1.65 REQUIRED COMPONENTS system)
	find_package(Threads REQUIRED)
	add_executable(playground "examples/playground.cpp")
	target_link_libraries(playground Boost::boost Boost::system Threads::Threads)
endif()

# install
include(CMakePackageConfigHelpers)

set(INCLUDE_INSTALL_DIR "include/")
set(LIBRARY_INSTALL_DIR "lib/${PROJECT_NAME}")

configure_package_config_file("${PROJECT_NAME}-config.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
	INSTALL_DESTINATION "lib/${PROJECT_NAME}/cmake"
	PATH_VARS INCLUDE_INSTALL_DIR LIBRARY_INSTALL_DIR)
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion)

install(TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}-target
	INCLUDES DESTINATION "${INCLUDE_INSTALL_DIR}")
install(EXPORT ${PROJECT_NAME}-target
	DESTINATION "lib/${PROJECT_NAME}/cmake"
	NAMESPACE ${PROJECT_NAME}::
	EXPORT_LINK_INTERFACE_LIBRARIES
	FILE "${PROJECT_NAME}-target.cmake")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
	DESTINATION "lib/${PROJECT_NAME}/cmake")
install(DIRECTORY ${PROJECT_NAME}
	DESTINATION include)
