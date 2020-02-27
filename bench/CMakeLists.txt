set(
  BENCHS
  simple_bench
)

# TODO: link realm library

function(add_cmake_bench target)
  string(CONCAT target_src ${target} ".cpp")
  add_executable(${target} ${target_src})
  target_link_libraries(${target} realm)
endfunction()

foreach(b ${BENCHS})
  add_cmake_bench(${b})
endforeach()

# Enable optimization for benchmarks
if(MSVC)
    add_compile_options("/Ox")
endif(MSVC)

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif(CMAKE_COMPILER_IS_GNUCXX)