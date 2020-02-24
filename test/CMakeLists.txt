set(
  TESTS
  entity_tests
  system_tests
  scheduler_tests
  singleton_tests
  example_test
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