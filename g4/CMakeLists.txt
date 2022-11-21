set(language_name "walang")

set(antlr4_gen_srcs
  ${CMAKE_CURRENT_SOURCE_DIR}/generated/${language_name}BaseListener.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/generated/${language_name}Listener.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/generated/${language_name}Lexer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/generated/${language_name}Parser.cpp
)

foreach(src_file ${antlr4_gen_srcs})
  set_source_files_properties(
    ${src_file}
    PROPERTIES
    GENERATED TRUE
  )
endforeach()

add_custom_target(GenerateParser DEPENDS ${antlr4_gen_srcs})
add_custom_command(
  OUTPUT ${antlr4_gen_srcs}
  COMMAND antlr4 -Dlanguage=Cpp ${CMAKE_CURRENT_SOURCE_DIR}/walang.g4 -o ${CMAKE_CURRENT_SOURCE_DIR}/generated
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/walang.g4
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building ${Name} with ANTLR4"
)

add_library(walang_parser STATIC ${antlr4_gen_srcs})
target_link_libraries(walang_parser PUBLIC antlr4_static)
target_include_directories(walang_parser PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/antlr/runtime/Cpp/runtime/src
)
