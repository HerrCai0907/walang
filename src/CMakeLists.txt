aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} srcs)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/ast ast_srcs)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/ir ir_srcs)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# include antlr
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/antlr/runtime/Cpp/runtime/src
)

# include magic_enum
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/magic_enum/include
)

# include fmt
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/fmt/include
)

# include binaryen
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/binaryen/src
)

# include generated code
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../g4/
)

add_executable(walang
  ${srcs}
  ${ast_srcs}
  ${ir_srcs}
)
target_link_libraries(walang
  walang_parser fmt::fmt binaryen
)
set_property(TARGET walang PROPERTY OUTPUT_NAME "${CMAKE_BINARY_DIR}/walang")
