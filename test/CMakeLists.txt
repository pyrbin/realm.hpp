cmake_minimum_required(VERSION 3.15)

project("pillar_testing")

add_executable(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} CONAN_PKG::Catch2)

target_sources(${PROJECT_NAME}
               PRIVATE
               test.cpp)