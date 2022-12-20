#include "binaryen-c.h"
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

TEST_F(CompileCallTest, ReturnValue) {
  FileParser parser("test.wa", R"(
function add(a:i32,b:i32):i32{
  return a+b;
}
let c = add(1,2);
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}
TEST_F(CompileCallTest, ReturnClass) {
  FileParser parser("test.wa", R"(
class A {}
class B {
  v:i32;
}
class C {
  v1:i32;
  v2:f64;
}
function createA():A{
  return A();
}
function createB():B{
  return B();
}
function createC():C{
  return C();
}
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}
TEST_F(CompileCallTest, ClassAsParameter) {
  FileParser parser("test.wa", R"(
class A {}
class B {
  v:i32;
}
class C {
  v1:i32;
  v2:f64;
}
function create(a:A,b:B,c:C):C{
  return C();
}
let a = A();
let b = B();
let c = C();
let v:C = create(a,b,c);
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
      ArgumentCountError);
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
      ArgumentCountError);
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
