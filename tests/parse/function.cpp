#include "ast/statement.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>

using namespace walang;
using namespace walang::ast;

TEST(ParseFunction, FunctionStatement) {
  FileParser parser("test.wa", R"(
function foo() {}
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<FunctionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "fn foo () -> __unknown__ {\n}\n");
}

TEST(ParseFunction, FunctionStatementWithArgument) {
  FileParser parser("test.wa", R"(
function foo(a1:t1,a2:t2) {}
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<FunctionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "fn foo (a1:t1, a2:t2) -> __unknown__ {\n}\n");
}

TEST(ParseFunction, FunctionStatementWithReturnType) {
  FileParser parser("test.wa", R"(
function foo() : i32 {}
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<FunctionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "fn foo () -> i32 {\n}\n");
}
