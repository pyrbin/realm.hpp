# reference: https://www.reddit.com/r/cpp/comments/c8ty2h/a_line_by_line_explanation_how_to_create_a/

cmake_minimum_required(VERSION 3.15)

project(
  realm
  VERSION 0.1.0
  LANGUAGES CXX
  DESCRIPTION
    "A header only ecs c++20 library"
)

add_library(${PROJECT_NAME} INTERFACE)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

include(CTest)

target_include_directories(
  ${PROJECT_NAME}
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# Settings
set(CMAKE_CXX_STANDARD 20)

# todo: Should flags be put in cmakelist?
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wall -o3")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Conan
find_program(CONAN conan)
if(NOT CONAN)
  message(WARNING "Conan not found. Skipping tests")
  set(BUILD_TESTING FALSE)
else()
  # setting up conan to download.cmake automatically, you can also just copy the
  # conan.cmake file
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(
      STATUS
        "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(
      DOWNLOAD
      "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
      "${CMAKE_BINARY_DIR}/conan.cmake")
  endif()

  include(${CMAKE_BINARY_DIR}/conan.cmake)
  conan_cmake_run(CONANFILE conanfile.txt BASIC_SETUP CMAKE_TARGETS BUILD
                  missing)
endif()

# Testing
if(BUILD_TESTING)
  add_subdirectory(test)
endif()