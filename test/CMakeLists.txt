set(
  TESTS
  entity
  query
  system
)

# TODO: link realm library

function(add_cmake_test target)
  string(CONCAT target_src ${target} ".cpp")
  add_executable(${target} ${target_src})
  add_test(NAME ${target} COMMAND ${target})
endfunction()

foreach(t ${TESTS})
  add_cmake_test(${t})
endforeach()