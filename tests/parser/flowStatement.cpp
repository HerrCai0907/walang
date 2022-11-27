#include "ast/statement.hpp"
#include "parser.hpp"
#include <fmt/core.h>
#include <gtest/gtest.h>

using namespace walang;
using namespace walang::ast;

TEST(ParseFlowStatement, Block) {
  FileParser parser("test.wa", R"(
{
  a+1;
  b+2;
}
  )");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<BlockStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), R"({
(ADD a 1)
(ADD b 2)
})");
}

TEST(ParseFlowStatement, If) {
  FileParser parser("test.wa", R"(
if (1) {}
  )");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<IfStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), R"(if 1 then {
})");
}

TEST(ParseFlowStatement, IfElse) {
  FileParser parser("test.wa", R"(
if (1) {
} else {
}
  )");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<IfStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), R"(if 1 then {
} else {
})");
}

TEST(ParseFlowStatement, ElseIf) {
  FileParser parser("test.wa", R"(
if (1) {
} else if (2) {
} else {
}
  )");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<IfStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), R"(if 1 then {
} else if 2 then {
} else {
})");
}

TEST(ParseFlowStatement, While) {
  FileParser parser("test.wa", R"(
while (1) {
}
  )");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<WhileStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), R"(while 1 {
})");
}
