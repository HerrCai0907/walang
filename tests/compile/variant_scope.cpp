#include "compiler.hpp"
#include "helper/snapshot.hpp"
#include "parser.hpp"
#include <binaryen-c.h>
#include <filesystem>
#include <gtest/gtest.h>

using namespace walang;
using namespace walang::ast;

class VariantScope : public ::testing::Test {
public:
  static test_helper::SnapShot snapshot;
};
test_helper::SnapShot VariantScope::snapshot{std::filesystem::path(__FILE__).replace_extension("xml")};

TEST_F(VariantScope, Basis) {
  FileParser parser("test.wa", R"(
function foo():void{
    {
        let i = 0;
    }
    {
        let i = 0;
    }
}
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}

TEST_F(VariantScope, Error) {
  EXPECT_THROW(
      [] {
        FileParser parser("test.wa", R"(
function foo():void{
    let i = 0;
    {
        let i = 0;
    }
}
    )");
        auto file = parser.parse();
        Compiler compile{{file}};
        compile.compile();
        ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
        snapshot.check(compile.wat());
      }(),
      RedefinedSymbol);
}
