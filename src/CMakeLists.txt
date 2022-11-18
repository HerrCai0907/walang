aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} srcs)

# include antlr
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/antlr/runtime/Cpp/runtime/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/antlr/runtime/Cpp/runtime/src/atn
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/antlr/runtime/Cpp/runtime/src/dfa
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/antlr/runtime/Cpp/runtime/src/internal
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/antlr/runtime/Cpp/runtime/src/misc
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/antlr/runtime/Cpp/runtime/src/support
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/antlr/runtime/Cpp/runtime/src/tree
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/antlr/runtime/Cpp/runtime/src/tree/pattern
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/antlr/runtime/Cpp/runtime/src/tree/xpath
)

# include generated code
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../g4/
)

add_executable(walang ${srcs})
target_link_libraries(walang walang_parser)
set_property(TARGET walang PROPERTY OUTPUT_NAME "${CMAKE_BINARY_DIR}/walang")
