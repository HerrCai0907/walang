set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/15460959cbbfa20e66ef0b5ab497367e47fc0a04.zip
)
set(gtest_force_shared_crt
  ON
  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
)

set(TESTCASES)

function(TESTCASE test_name)
  message(STATUS "test case ${test_name}")
  add_executable(${test_name} ${ARGN})
  target_link_libraries(${test_name} walang-core GTest::gtest_main tinyxml2)
  gtest_discover_tests(${test_name})
  set(TESTCASES ${TESTCASES} ${test_name} PARENT_SCOPE)
endfunction()

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/parse parse_test_srcs)
TESTCASE(parseTest ${parse_test_srcs})

TESTCASE(compileTest
  compile/basisStatement.cpp
  compile/ifStatement.cpp
  compile/whileStatement.cpp
  compile/functionStatement.cpp
  compile/call.cpp
)

if(ENABLE_COV)
  set(CMAKE_BUILD_TYPE "Debug")
  include(CodeCoverage)
  APPEND_COVERAGE_COMPILER_FLAGS()

  set(COVERAGE_LCOV_EXCLUDES
    "/usr/include/*"
    "/**/*/usr/include/*"
    "${gtest_SOURCE_DIR}/*"
    "${PROJECT_SOURCE_DIR}/tests/*"
    "${PROJECT_SOURCE_DIR}/g4/*"
    "${PROJECT_SOURCE_DIR}/third_party/*"
  )

  SETUP_TARGET_FOR_COVERAGE_LCOV(NAME coverage
    EXECUTABLE ctest
    DEPENDENCIES ${TESTCASES}
    LCOV_ARGS --rc lcov_branch_coverage=1
    GENHTML_ARGS --rc lcov_branch_coverage=1
  )
endif()