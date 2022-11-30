#include "ast/statement.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace walang;
using namespace walang::ast;

TEST(ParserPrefixExpression, not_op) {
  FileParser parser("test.wa", R"(
not a;
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "(NOT a)\n");
}

TEST(ParserPrefixExpression, priority) {
  FileParser parser("test.wa", R"(
1 + not a + 2;
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "(ADD (ADD 1 (NOT a)) 2)\n");
}

TEST(ParserPrefixExpression, parentheses) {
  FileParser parser("test.wa", R"(
not(1 + 2);
  )");
  auto file = parser.parse();
  ASSERT_EQ(file->statement()[0]->to_string(), "(NOT (ADD 1 2))\n");
}
