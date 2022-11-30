#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>

using namespace walang;
using namespace walang::ast;

TEST(ParseCallExpression, basis) {
  FileParser parser("test.wa", R"(
foo(a,b,1,2.5);
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "foo(a, b, 1, 2.5)\n");
}

TEST(ParseCallExpression, repeat) {
  FileParser parser("test.wa", R"(
foo(a)(b)(c);
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "foo(a)(b)(c)\n");
}