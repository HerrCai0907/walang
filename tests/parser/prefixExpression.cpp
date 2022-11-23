#include "ast/statement.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace walang;
using namespace walang::ast;

TEST(parser_prefix_expression, not_op) {
  FileParser parser("test.wa", R"(
not a;
  )");
  auto file = parser.parse();

  EXPECT_EQ(file->statement().size(), 1);
  EXPECT_NE(std::dynamic_pointer_cast<ExpressionStatement>(file->statement()[0]), nullptr);
  EXPECT_EQ(file->statement()[0]->to_string(), "(NOT a)\n");
}