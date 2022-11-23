#include "ast/op.hpp"
#include "expression.hpp"
#include <cassert>
#include <fmt/core.h>
#include <memory>
#include <string>
#include <variant>

namespace walang {
namespace ast {

PrefixExpression::PrefixExpression(
    walangParser::PrefixExpressionContext *ctx,
    std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map) {
  this->op_ = Operator::getOp(ctx->prefixOperator());
  assert(map.count(ctx->expression()) == 1);
  this->expr_ = std::dynamic_pointer_cast<Expression>(map.find(ctx->expression())->second);
}

std::string PrefixExpression::to_string() const {
  return fmt::format("({0} {1})", Operator::to_string(op_), expr_->to_string());
}

} // namespace ast
} // namespace walang