#include "../helper/snapshot.hpp"
#include "binaryen-c.h"
#include "compiler.hpp"
#include "parser.hpp"
#include <filesystem>
#include <gtest/gtest.h>

using namespace walang;
using namespace walang::ast;

class CompileWhileStatementTest : public ::testing::Test {
public:
  static test_helper::SnapShot snapshot;
};
test_helper::SnapShot CompileWhileStatementTest::snapshot{
    std::filesystem::path(__FILE__).parent_path().append("compile_while_statement.snapshot")};

TEST_F(CompileWhileStatementTest, Basis) {
  FileParser parser("test.wa", R"(
while (1) {
  2;
}
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}

TEST_F(CompileWhileStatementTest, Break) {
  FileParser parser("test.wa", R"(
while (1) {
  break;
}
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}

TEST_F(CompileWhileStatementTest, Continue) {
  FileParser parser("test.wa", R"(
while (1) {
  continue;
}
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}

TEST_F(CompileWhileStatementTest, MutipleLevlBreak) {
  FileParser parser("test.wa", R"(
while (1) {
  while (2) {
    break;
  }
  break;
}
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}

TEST_F(CompileWhileStatementTest, MutipleLevlContinue) {
  FileParser parser("test.wa", R"(
while (1) {
  while (2) {
    continue;
  }
  continue;
}
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}
