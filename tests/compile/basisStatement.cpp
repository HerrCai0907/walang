#include "../helper/snapshot.hpp"
#include "binaryen-c.h"
#include "compiler.hpp"
#include "parser.hpp"
#include <filesystem>
#include <gtest/gtest.h>
#include <stdexcept>

using namespace walang;
using namespace walang::ast;

class CompileBasisStatementTest : public ::testing::Test {
public:
  static test_helper::SnapShot snapshot;
};
test_helper::SnapShot CompileBasisStatementTest::snapshot{
    std::filesystem::path(__FILE__).parent_path().append("compile_basis_statement.snapshot.xml")};

TEST_F(CompileBasisStatementTest, binaryExpression) {
  FileParser parser("test.wa", R"(
1 >> 2;
let a : u32 = 3 >> 4;
let b : f64 = 5 * 6;
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
  ASSERT_THROW(
      [] {
        FileParser parser("test.wa", R"(
let a : f32 = 0 && 4;
    )");
        auto file = parser.parse();
        Compiler compile{{file}};
        compile.compile();
      }(),
      std::runtime_error);
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
  ASSERT_THROW(
      [] {
        FileParser parser("test.wa", R"(
let a : f32 = 0 || 4;
    )");
        auto file = parser.parse();
        Compiler compile{{file}};
        compile.compile();
      }(),
      std::runtime_error);
}

TEST_F(CompileBasisStatementTest, prefixExpression) {
  FileParser parser("test.wa", R"(
let a = 0;
+a;
-a;
not a;
let b:u64 = 0;
+b;
-b;
not b;
let c = 0.0;
+c;
-c;
let d:f64 = 0;
+d;
-d;
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}
TEST_F(CompileBasisStatementTest, prefixExpressionFailed) {
  ASSERT_THROW(
      [] {
        FileParser parser("test.wa", R"(
let a:f32 = 0;
not a;
    )");
        auto file = parser.parse();
        Compiler compile{{file}};
        compile.compile();
      }(),
      std::runtime_error);
  ASSERT_THROW(
      [] {
        FileParser parser("test.wa", R"(
let a:f64 = 0;
not a;
    )");
        auto file = parser.parse();
        Compiler compile{{file}};
        compile.compile();
      }(),
      std::runtime_error);
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

TEST_F(CompileBasisStatementTest, DeclareStatement) {
  FileParser parser("test.wa", R"(
let a = 1;
let b : i32 = 2;
let c : u32 = 3;
let d : i64 = 4;
let e : u64 = 5;
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}
