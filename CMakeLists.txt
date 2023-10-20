cmake_minimum_required(VERSION 3.16)

project(
  terraqtt
  VERSION 0.4.0
  LANGUAGES CXX
)

if(CMAKE_VERSION VERSION_LESS 3.21)
  get_property(
    parent
    DIRECTORY
    PROPERTY PARENT_DIRECTORY
  )
  if(NOT parent)
    set(PROJECT_IS_TOP_LEVEL TRUE)
  endif()
endif()

option(TERRAQTT_BUILD_EXAMPLES "Build some example projects." ON)
option(TERRAQTT_LOG_ENABLE "Enables logging output." OFF)
set(TERRAQTT_LOG_LEVEL
    "INFO"
    CACHE STRING "The logging level (CRITICAL, ERROR, WARN, INFO, DEBUG, TRACE)"
)
set_property(
  CACHE TERRAQTT_LOG_LEVEL
  PROPERTY STRINGS
           "CRITICAL"
           "ERROR"
           "WARN"
           "INFO"
           "DEBUG"
           "TRACE"
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/terraqtt/config.hpp"
)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>")
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

if(TERRAQTT_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(TERRAQTT_TEST_ENABLE)
  # add_subdirectory(tests)
endif()

if(TERRAQTT_LOG_ENABLE)
  find_package(spdlog REQUIRED)
  target_link_libraries(${PROJECT_NAME} INTERFACE spdlog::spdlog)
endif()

# install
include(CMakePackageConfigHelpers)

set(INCLUDE_INSTALL_DIR "include/")
set(LIBRARY_INSTALL_DIR "lib/${PROJECT_NAME}")

configure_package_config_file(
  "${PROJECT_NAME}-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
  INSTALL_DESTINATION "lib/${PROJECT_NAME}/cmake"
  PATH_VARS INCLUDE_INSTALL_DIR LIBRARY_INSTALL_DIR
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}-target
  INCLUDES
  DESTINATION "${INCLUDE_INSTALL_DIR}"
)
install(
  EXPORT ${PROJECT_NAME}-target
  DESTINATION "lib/${PROJECT_NAME}/cmake"
  NAMESPACE ${PROJECT_NAME}::
  EXPORT_LINK_INTERFACE_LIBRARIES
  FILE "${PROJECT_NAME}-target.cmake"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
        DESTINATION "lib/${PROJECT_NAME}/cmake"
)
install(DIRECTORY ${PROJECT_NAME} DESTINATION include)
