aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} srcs)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(walang
  ${srcs}
)
target_link_libraries(walang walang-core)
set_property(TARGET walang PROPERTY OUTPUT_NAME "${CMAKE_BINARY_DIR}/walang")
