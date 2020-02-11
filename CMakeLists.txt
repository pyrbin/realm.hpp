# reference: https://www.reddit.com/r/cpp/comments/c8ty2h/a_line_by_line_explanation_how_to_create_a/

cmake_minimum_required(VERSION 3.15)

project(
  realm
  VERSION "0.1.0"
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
option(REALM_DOCS "Create documentation" ${IS_MASTER_PROJECT})

# Targets
if(REALM_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

#if(REALM_DOCS)
#  add_subdirectory(docs)
#endif()

# Library
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

target_include_directories(
  ${PROJECT_NAME}
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/realm.hpp>)
