#include "../helper/snapshot.hpp"
#include "binaryen-c.h"
#include "compiler.hpp"
#include "parser.hpp"
#include <filesystem>
#include <gtest/gtest.h>

using namespace walang;
using namespace walang::ast;

class CompileIfStatementTest : public ::testing::Test {
public:
  static test_helper::SnapShot snapshot;
};
test_helper::SnapShot CompileIfStatementTest::snapshot{
    std::filesystem::path(__FILE__).parent_path().append("compile_if_statement.snapshot")};

TEST_F(CompileIfStatementTest, basis) {
  FileParser parser("test.wa", R"(
let a = 0;
if (a) {
  a = 1;
}
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}

TEST_F(CompileIfStatementTest, IfWithElse) {
  FileParser parser("test.wa", R"(
let a = 0;
if (a) {
  a = 1;
} else {
  a = 2;
}
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}

TEST_F(CompileIfStatementTest, IfWithElseIf) {
  FileParser parser("test.wa", R"(
let a = 0;
if (a) {
  a = 1;
} else if (a) {
  a = 2;
} else {
  a = 3;
}
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}
