aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} core_srcs)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/ast ast_srcs)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/ir ir_srcs)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/helper helper_srcs)

if(ENABLE_CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY clang-tidy -p ${CMAKE_CURRENT_SOURCE_DIR} "--header-filter='!((*/third_party/*)|(*/g4/*))'")
endif(ENABLE_CLANG_TIDY)

add_library(walang-core
  ${core_srcs}
  ${ast_srcs}
  ${ir_srcs}
  ${helper_srcs}
)

# include generated code
target_include_directories(walang-core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../g4/
)
target_include_directories(walang-core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(walang-core
  walang_parser fmt::fmt binaryen
)

add_subdirectory(cli)

if(ENABLE_COV)
  set(CMAKE_BUILD_TYPE "Debug")
  include(CodeCoverage)
  APPEND_COVERAGE_COMPILER_FLAGS()
endif()
