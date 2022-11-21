cmake_minimum_required(VERSION 3.13.4)
project(walang)

# configuration
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
list(APPEND CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake-modules)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# dependiences
set(ANTLR_BUILD_CPP_TESTS 0)
add_subdirectory(third_party/antlr/runtime/Cpp)
unset(ANTLR_BUILD_CPP_TESTS)

add_subdirectory(third_party/fmt)

set(BUILD_TESTS off)
set(BUILD_TOOLS off)
add_subdirectory(third_party/binaryen)
unset(BUILD_TESTS)
unset(BUILD_TOOLS)

# include magic_enum
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/magic_enum/include
)

# include fmt
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmt/include
)

# include binaryen
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/binaryen/src
)

# source
add_subdirectory(g4)
add_subdirectory(src)

# test
if(NOT DEFINED EMBEDDED_SERIALIZATION_INTEGRATION)
  if(NOT DISABLE_TEST)
    enable_testing()
    add_subdirectory(tests)
  endif()
else()
  message(STATUS "EMBEDDED_SERIALIZATION_INTEGRATION mode")
endif()
