#include "compiler.hpp"
#include "helper/diagnose.hpp"
#include "helper/snapshot.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>

using namespace walang;
using namespace walang::ast;

class CompileClassTest : public ::testing::Test {
public:
  static test_helper::SnapShot snapshot;
};
test_helper::SnapShot CompileClassTest::snapshot{std::filesystem::path(__FILE__).replace_extension("xml")};

TEST_F(CompileClassTest, Basis) {
  FileParser parser("test.wa", R"(
class A {
  memberA:i32;
  memberB:f32;
  function foo(): void {}
  function foo2(a:i32): void {}
}
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}
TEST_F(CompileClassTest, Declare) {
  FileParser parser("test.wa", R"(
class A {
  a : f64;
  b : i32;
}
class B {}
class C {
  c : i64;
}
let ga = A();
let gb = B();
let gc = C();
function foo():void{
  let la = A();
  let lb = B();
  let lc = C();
}
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}
TEST_F(CompileClassTest, CallMethod) {
  FileParser parser("test.wa", R"(
class A {
  a1 : f64;
  function setA():void{
    this.a1 = 10;
  }
}
class B {
  b1 : f64;
  b2 : i32;
  function setA():void{
    this.b1 = 10;
    this.b2 = 11;
  }
}
    )");
  auto file = parser.parse();
  Compiler compile{{file}};
  compile.compile();
  ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
  snapshot.check(compile.wat());
}

TEST_F(CompileClassTest, Error) {
  EXPECT_THROW(
      [] {
        FileParser parser("test.wa", R"(
class A {
  a:i32;
  a:f32;
}
    )");
        auto file = parser.parse();
        Compiler compile{{file}};
        compile.compile();
        ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
        snapshot.check(compile.wat());
      }(),
      RedefinedSymbol);
  EXPECT_THROW(
      [] {
        FileParser parser("test.wa", R"(
class A {
  a:i32;
  function a(): void {}
}
    )");
        auto file = parser.parse();
        Compiler compile{{file}};
        compile.compile();
        ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
        snapshot.check(compile.wat());
      }(),
      RedefinedSymbol);
  EXPECT_THROW(
      [] {
        FileParser parser("test.wa", R"(
class A {
  function a(): void {}
  function a(): void {}
}
    )");
        auto file = parser.parse();
        Compiler compile{{file}};
        compile.compile();
        ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
        snapshot.check(compile.wat());
      }(),
      RedefinedSymbol);

  EXPECT_THROW(
      [] {
        FileParser parser("test.wa", R"(
class A {
  a : A;
}
    )");
        auto file = parser.parse();
        Compiler compile{{file}};
        compile.compile();
        ASSERT_TRUE(BinaryenModuleValidate(compile.module()));
        snapshot.check(compile.wat());
      }(),
      RecursiveDefinedSymbol);
}
