#include "ast/statement.hpp"
#include "parser.hpp"
#include <fmt/core.h>
#include <gtest/gtest.h>

using namespace walang;
using namespace walang::ast;

TEST(ParseIfStatement, If) {
  FileParser parser("test.wa", R"(
if (1) {}
  )");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<IfStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), R"(if 1 then {
})");
}

TEST(ParseIfStatement, IfElse) {
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

TEST(ParseIfStatement, ElseIf) {
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
