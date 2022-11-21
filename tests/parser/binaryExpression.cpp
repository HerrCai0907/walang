#include "ast/statement.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace walang;
using namespace walang::ast;

TEST(parser_binary_expression, add) {
  FileParser parser("test.wa", R"(
a + 4;
  )");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  EXPECT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), "(ADD a 4)\n");
}
TEST(parser_binary_expression, sub) {
  FileParser parser("test.wa", R"(
a - 4;
  )");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  EXPECT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), "(SUB a 4)\n");
}
TEST(parser_binary_expression, mul) {
  FileParser parser("test.wa", R"(
a * 4;
  )");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  EXPECT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), "(MUL a 4)\n");
}
TEST(parser_binary_expression, div) {
  FileParser parser("test.wa", R"(
a / 4;
  )");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  EXPECT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), "(DIV a 4)\n");
}
TEST(parser_binary_expression, mod) {
  FileParser parser("test.wa", R"(
a % 4;
  )");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  EXPECT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), "(MOD a 4)\n");
}
TEST(parser_binary_expression, shift) {
  FileParser parser("test.wa", R"(
a >> 4;
a << 4;
  )");
  auto file = parser.parse();
  EXPECT_EQ(file->statement().size(), 2);
  for (auto statement : file->statement()) {
    ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(statement), nullptr);
  }
  EXPECT_EQ(file->statement()[0]->to_string(), "(RIGHT_SHIFT a 4)\n");
  EXPECT_EQ(file->statement()[1]->to_string(), "(LEFT_SHIFT a 4)\n");
}
TEST(parser_binary_expression, equal) {
  FileParser parser("test.wa", R"(
a > 4;
a < 4;
a >= 4;
a <= 4;
a == 4;
a != 4;
  )");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 6);
  for (auto statement : file->statement()) {
    ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(statement), nullptr);
  }
  EXPECT_EQ(file->statement()[0]->to_string(), "(GREATER_THAN a 4)\n");
  EXPECT_EQ(file->statement()[1]->to_string(), "(LESS_THAN a 4)\n");
  EXPECT_EQ(file->statement()[2]->to_string(), "(NO_LESS_THAN a 4)\n");
  EXPECT_EQ(file->statement()[3]->to_string(), "(NO_GREATER_THAN a 4)\n");
  EXPECT_EQ(file->statement()[4]->to_string(), "(EQUAL a 4)\n");
  EXPECT_EQ(file->statement()[5]->to_string(), "(NOT_EQUAL a 4)\n");
}
