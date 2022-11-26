#include "../helper/snapshot.hpp"
#include "binaryen-c.h"
#include "compiler.hpp"
#include "parser.hpp"
#include <array>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

using namespace walang;
using namespace walang::ast;

const char *getCurrentTestName() {
  const testing::TestInfo *const test_info = testing::UnitTest::GetInstance()->current_test_info();
  return test_info->name();
}

std::string wasm2wat(BinaryenModuleRef module) {
  std::array<char, 65535> wat{};
  BinaryenSetColorsEnabled(false);
  auto size = BinaryenModuleWriteText(module, wat.data(), wat.size());
  return std::string{wat.data(), size};
}

class compileBasisStatement : public ::testing::Test {
public:
  static void TearDownTestSuite() {
    if (testing::UnitTest::GetInstance()->failed_test_suite_count() == 0) {
      snapshot.removeUsedItem();
    }
  }
  static test_helper::SnapShot snapshot;
};
test_helper::SnapShot compileBasisStatement::snapshot{
    std::filesystem::path(__FILE__).parent_path().append("compile_basis_statement.snapshot")};

TEST_F(compileBasisStatement, binaryExpression) {
  FileParser parser("test.wa", R"(
  1 << 4;
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(getCurrentTestName(), wasm2wat(compile.module()));
}

TEST_F(compileBasisStatement, logicAndExpression) {
  FileParser parser("test.wa", R"(
  0 && 4;
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(getCurrentTestName(), wasm2wat(compile.module()));
}

TEST_F(compileBasisStatement, logicOrExpression) {
  FileParser parser("test.wa", R"(
  1 || 5;
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(getCurrentTestName(), wasm2wat(compile.module()));
}

TEST_F(compileBasisStatement, prefixExpression) {
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
  snapshot.check(getCurrentTestName(), wasm2wat(compile.module()));
}
