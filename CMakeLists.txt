cmake_minimum_required(VERSION 3.15)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Projects Settings                                               │
# └──────────────────────────────────────────────────────────────────┘

project(
  realm
  VERSION "0.0.1"
  LANGUAGES CXX
  DESCRIPTION
  "A header only ecs c++20/17 library"
)

set(IS_MASTER_PROJECT OFF)

if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  set(IS_MASTER_PROJECT ON)
endif()

# Enable if master project
option(REALM_TESTS "Build unit tests" ${IS_MASTER_PROJECT})
option(REALM_BENCH "Build benchmarks" ${IS_MASTER_PROJECT})

# Library
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(
  ${PROJECT_NAME}
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# Settings
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

# Targets
if(REALM_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()

if(REALM_BENCH)
  add_subdirectory(bench)
endif()