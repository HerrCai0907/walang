aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} core_srcs)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/ast ast_srcs)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/ir ir_srcs)

add_library(walang-core
  ${core_srcs}
  ${ast_srcs}
  ${ir_srcs}
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