#include "../helper/snapshot.hpp"
#include "binaryen-c.h"
#include "compiler.hpp"
#include "parser.hpp"
#include <filesystem>
#include <gtest/gtest.h>

using namespace walang;
using namespace walang::ast;

class CompileBasisStatementTest : public ::testing::Test {
public:
  static test_helper::SnapShot snapshot;
};
test_helper::SnapShot CompileBasisStatementTest::snapshot{
    std::filesystem::path(__FILE__).parent_path().append("compile_basis_statement.snapshot")};

TEST_F(CompileBasisStatementTest, binaryExpression) {
  FileParser parser("test.wa", R"(
  1 << 4;
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}

TEST_F(CompileBasisStatementTest, logicAndExpression) {
  FileParser parser("test.wa", R"(
  0 && 4;
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}

TEST_F(CompileBasisStatementTest, logicOrExpression) {
  FileParser parser("test.wa", R"(
  1 || 5;
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}

TEST_F(CompileBasisStatementTest, prefixExpression) {
  FileParser parser("test.wa", R"(
  let a = 0;
  +a;
  -a;
  not a;
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}

TEST_F(CompileBasisStatementTest, ternaryExpression) {
  FileParser parser("test.wa", R"(
  1 ? 0 : 2;
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}
