#include "ast/statement.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>

using namespace walang;
using namespace walang::ast;

TEST(ParseClass, basis) {
  FileParser parser("test.wa", R"(
class foo {
}
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ClassStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "class foo {\n}\n");
}
TEST(ParseClass, withMember) {
  FileParser parser("test.wa", R"(
class foo {
  a:i32;
  b:f64;
}
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ClassStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "class foo {\na:i32\nb:f64\n}\n");
}
TEST(ParseClass, withFunction) {
  FileParser parser("test.wa", R"(
class foo {
  function a():i32{}
  function b():f32{}
}
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ClassStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "class foo {\nfn a () -> i32 {\n}\nfn b () -> f32 {\n}\n}\n");
}
