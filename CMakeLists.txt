cmake_minimum_required(VERSION 3.1)

project(terraqtt
	VERSION 0.2.0
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

option(TERRAQTT_BUILD_EXAMPLES "Build some example projects." ON)
option(TERRAQTT_LOG_ENABLE "Enables logging output." OFF)
set(TERRAQTT_LOG_LEVEL "INFO" CACHE STRING "The logging level (CIRITICAL, ERROR, WARN, INFO, DEBUG, TRACE)")
set_property(CACHE TERRAQTT_LOG_LEVEL PROPERTY STRINGS "CRITICAL" "ERROR" "WARN" "INFO" "DEBUG" "TRACE")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/terraqtt/config.hpp")

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

if(TERRAQTT_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(TERRAQTT_LOG_ENABLE)
	find_package(spdlog REQUIRED)
	target_link_libraries(${PROJECT_NAME} INTERFACE spdlog::spdlog)
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
