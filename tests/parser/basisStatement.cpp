#include "ast/statement.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>

using namespace walang;
using namespace walang::ast;

TEST(ParseBasisStatement, DeclareStatement) {
  FileParser parser("test.wa", "let a = 4;");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<DeclareStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), "declare 'a' <- 4\n");
}

TEST(ParseBasisStatement, TypedDeclareStatement) {
  FileParser parser("test.wa", "let a:i32 = 4;");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<DeclareStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), "declare i32'a' <- 4\n");
}

TEST(ParseBasisStatement, ExpressionStatement) {
  FileParser parser("test.wa", "4;");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), "4\n");
}

TEST(ParseBasisStatement, AssignStatement) {
  FileParser parser("test.wa", "a = 4;");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  ASSERT_NE(std::dynamic_pointer_cast<AssignStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), "a <- 4\n");
}
