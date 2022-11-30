#include "ast/statement.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace walang;
using namespace walang::ast;

TEST(ParserTernaryExpression, basis) {
  FileParser parser("test.wa", R"(
a ? 1 : 2;
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "(a ? 1 : 2)\n");
}

TEST(ParserTernaryExpression, multiple) {
  FileParser parser("test.wa", R"(
1 ? 2 : 3 ? 4 : 5;
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "(1 ? 2 : (3 ? 4 : 5))\n");
}

TEST(ParserTernaryExpression, priority) {
  FileParser parser("test.wa", R"(
1 == 2 ? a * b : 3 + 4;
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "((EQUAL 1 2) ? (MUL a b) : (ADD 3 4))\n");
}

TEST(ParserTernaryExpression, parentheses) {
  FileParser parser("test.wa", R"(
(a ? 1 : 2) + 1;
  )");
  auto file = parser.parse();
  ASSERT_EQ(file->statement()[0]->to_string(), "(ADD (a ? 1 : 2) 1)\n");
}
