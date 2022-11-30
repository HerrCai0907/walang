#include "compiler.hpp"
#include "helper/snapshot.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>
#include <stdexcept>

using namespace walang;
using namespace walang::ast;

class CompileFunctionStatementTest : public ::testing::Test {
public:
  static test_helper::SnapShot snapshot;
};
test_helper::SnapShot CompileFunctionStatementTest::snapshot{std::filesystem::path(__FILE__).replace_extension("xml")};

TEST_F(CompileFunctionStatementTest, basis) {
  FileParser parser("test.wa", R"(
function foo(a:i64, b: f32) : void {
  let c = 10;
  c = c + 1;
}
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}

TEST_F(CompileFunctionStatementTest, NoReturn) {
  // TODO(refactor after type infer)
  ASSERT_THROW(
      [] {
        FileParser parser("test.wa", R"(
function foo(a:i32, b: f32) {}
    )");
        auto file = parser.parse();
        Compiler compile{{file}};
        compile.compile();
      }(),
      std::runtime_error);
}
