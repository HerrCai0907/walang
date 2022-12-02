#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>

using namespace walang;
using namespace walang::ast;

TEST(ParseMemberExpression, Basis) {
  FileParser parser("test.wa", R"(
a.b;
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "a.b\n");
}
TEST(ParseMemberExpression, Priority) {
  FileParser parser("test.wa", R"(
a.b + 1;
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "(ADD a.b 1)\n");
}
TEST(ParseMemberExpression, PriorityWithCall) {
  FileParser parser("test.wa", R"(
a.b(c.d,e.f);
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "a.b(c.d, e.f)\n");
}
