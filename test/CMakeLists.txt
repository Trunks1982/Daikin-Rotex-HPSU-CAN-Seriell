cmake_minimum_required(VERSION 3.14)
project(HPSUTests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ESPHome release whose headers are used (fixed, so the tests do not break by themselves)
set(ESPHOME_GIT_TAG "2026.11.0" CACHE STRING "ESPHome release tag")

# Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ESPHome (headers only)
FetchContent_Declare(
    esphome_src
    GIT_REPOSITORY https://github.com/esphome/esphome.git
    GIT_TAG ${ESPHOME_GIT_TAG}
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(esphome_src)
if(NOT esphome_src_POPULATED)
    FetchContent_Populate(esphome_src)
endif()
set(ESPHOME_INCLUDE_ROOT ${esphome_src_SOURCE_DIR})

# The sources include "esphome/components/daikin_rotex_can/...": provide that path
set(COMPONENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../components/daikin_rotex_can)
set(INCLUDE_ROOT ${CMAKE_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${INCLUDE_ROOT}/esphome/components)
file(CREATE_LINK ${COMPONENT_DIR} ${INCLUDE_ROOT}/esphome/components/daikin_rotex_can SYMBOLIC)

# main

add_executable(hpsu_tests
    src/test_bidi_map.cpp
    src/test_pid.cpp
    src/test_scheduler.cpp
    src/test_utils.cpp
    ${COMPONENT_DIR}/scheduler.cpp
    ${COMPONENT_DIR}/pid.cpp
    ${COMPONENT_DIR}/bidi_map.cpp
    ${COMPONENT_DIR}/utils.cpp
    mock_esphome.cpp
)

target_include_directories(hpsu_tests PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include   # minimal esphome/core/defines.h, before ESPHome's own
    ${INCLUDE_ROOT}
    ${ESPHOME_INCLUDE_ROOT}
    ${COMPONENT_DIR}
)

target_link_libraries(hpsu_tests
    gtest_main
)

enable_testing()
include(GoogleTest)
gtest_discover_tests(hpsu_tests)
