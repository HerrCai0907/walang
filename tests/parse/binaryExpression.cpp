#include "ast/statement.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace walang;
using namespace walang::ast;

TEST(ParserBinaryExpression, add) {
  FileParser parser("test.wa", R"(
a + 4;
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "(ADD a 4)\n");
}
TEST(ParserBinaryExpression, sub) {
  FileParser parser("test.wa", R"(
a - 0x4;
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "(SUB a 4)\n");
}
TEST(ParserBinaryExpression, mul) {
  FileParser parser("test.wa", R"(
a * 4.2;
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "(MUL a 4.2)\n");
}
TEST(ParserBinaryExpression, div) {
  FileParser parser("test.wa", R"(
a / 4.1;
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "(DIV a 4.1)\n");
}
TEST(ParserBinaryExpression, mod) {
  FileParser parser("test.wa", R"(
a % 4;
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  ASSERT_EQ(file->statement()[0]->to_string(), "(MOD a 4)\n");
}
TEST(ParserBinaryExpression, shift) {
  FileParser parser("test.wa", R"(
a >> 4;
a << 4;
  )");
  auto file = parser.parse();
  ASSERT_EQ(file->statement().size(), 2);
  for (auto statement : file->statement()) {
    ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(statement), nullptr);
  }
  ASSERT_EQ(file->statement()[0]->to_string(), "(RIGHT_SHIFT a 4)\n");
  ASSERT_EQ(file->statement()[1]->to_string(), "(LEFT_SHIFT a 4)\n");
}
TEST(ParserBinaryExpression, equal) {
  FileParser parser("test.wa", R"(
a > 4;
a < 4;
a >= 4;
a <= 4;
a == 4;
a != 4;
  )");
  auto file = parser.parse();

  ASSERT_EQ(file->statement().size(), 6);
  for (auto statement : file->statement()) {
    ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(statement), nullptr);
  }
  ASSERT_EQ(file->statement()[0]->to_string(), "(GREATER_THAN a 4)\n");
  ASSERT_EQ(file->statement()[1]->to_string(), "(LESS_THAN a 4)\n");
  ASSERT_EQ(file->statement()[2]->to_string(), "(NO_LESS_THAN a 4)\n");
  ASSERT_EQ(file->statement()[3]->to_string(), "(NO_GREATER_THAN a 4)\n");
  ASSERT_EQ(file->statement()[4]->to_string(), "(EQUAL a 4)\n");
  ASSERT_EQ(file->statement()[5]->to_string(), "(NOT_EQUAL a 4)\n");
}

TEST(ParserBinaryExpression, priority_1) {
  FileParser parser("test.wa", R"(
a * 2 + 3;
  )");
  auto file = parser.parse();
  ASSERT_EQ(file->statement()[0]->to_string(), "(ADD (MUL a 2) 3)\n");
}
TEST(ParserBinaryExpression, priority_2) {
  FileParser parser("test.wa", R"(
a + 2 * 3;
  )");
  auto file = parser.parse();
  ASSERT_EQ(file->statement()[0]->to_string(), "(ADD a (MUL 2 3))\n");
}
TEST(ParserBinaryExpression, priority_3) {
  FileParser parser("test.wa", R"(
a + 2 + 3 + 4;
  )");
  auto file = parser.parse();
  ASSERT_EQ(file->statement()[0]->to_string(), "(ADD (ADD (ADD a 2) 3) 4)\n");
}
TEST(ParserBinaryExpression, priority_4) {
  FileParser parser("test.wa", R"(
a > a + 2 * 3;
  )");
  auto file = parser.parse();
  ASSERT_EQ(file->statement()[0]->to_string(), "(GREATER_THAN a (ADD a (MUL 2 3)))\n");
}
TEST(ParserBinaryExpression, priority_5) {
  FileParser parser("test.wa", R"(
a * 1 + 2 > 3;
  )");
  auto file = parser.parse();
  ASSERT_EQ(file->statement()[0]->to_string(), "(GREATER_THAN (ADD (MUL a 1) 2) 3)\n");
}
TEST(ParserBinaryExpression, parentheses) {
  FileParser parser("test.wa", R"(
a * (1 + 2);
  )");
  auto file = parser.parse();
  ASSERT_EQ(file->statement()[0]->to_string(), "(MUL a (ADD 1 2))\n");
}
