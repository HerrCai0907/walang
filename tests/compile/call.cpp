#include "compiler.hpp"
#include "helper/diagnose.hpp"
#include "helper/snapshot.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>
#include <stdexcept>

using namespace walang;
using namespace walang::ast;

class CompileCallTest : public ::testing::Test {
public:
  static test_helper::SnapShot snapshot;
};
test_helper::SnapShot CompileCallTest::snapshot{std::filesystem::path(__FILE__).replace_extension("xml")};

TEST_F(CompileCallTest, Basis) {
  FileParser parser("test.wa", R"(
function foo1():void{}
function foo2(a:i32,b:f64):void{}
foo1();
let v:f64 = 2.5;
foo2(1,v);
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}

TEST_F(CompileCallTest, ArgumentNotMatch) {
  EXPECT_THROW(
      [] {
        FileParser parser("test.wa", R"(
function foo():void{}
foo(1);
    )");
        auto file = parser.parse();
        Compiler compile{{file}};
        compile.compile();
      }(),
      std::runtime_error);
  EXPECT_THROW(
      [] {
        FileParser parser("test.wa", R"(
function foo(a:i32):void{}
foo();
    )");
        auto file = parser.parse();
        Compiler compile{{file}};
        compile.compile();
      }(),
      std::runtime_error);
  EXPECT_THROW(
      [] {
        FileParser parser("test.wa", R"(
function foo(a:i32):void{}
foo(1.5);
    )");
        auto file = parser.parse();
        Compiler compile{{file}};
        compile.compile();
      }(),
      TypeConvertError);
}

TEST_F(CompileCallTest, ReturnTypeNotMatch) {
  EXPECT_THROW(
      [] {
        FileParser parser("test.wa", R"(
function foo():void{}
let a = 1;
a = foo();
    )");
        auto file = parser.parse();
        Compiler compile{{file}};
        compile.compile();
      }(),
      TypeConvertError);
}
