#include "ast/op.hpp"
#include "expression.hpp"
#include <cassert>
#include <fmt/core.h>
#include <memory>
#include <string>
#include <variant>

namespace walang {
namespace ast {

TernaryExpression::TernaryExpression() noexcept
    : Expression(Type::TernaryExpression), conditionExpr_(nullptr), leftExpr_(nullptr), rightExpr_(nullptr) {}

TernaryExpression::TernaryExpression(walangParser::TernaryExpressionContext *ctx,
                                     std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map)
    : Expression(Type::TernaryExpression) {
  antlr4::ParserRuleContext *conditionCtx =
      dynamic_cast<antlr4::ParserRuleContext *>(ctx->ternaryExpressionCondition()->children.at(0));
  assert(map.count(conditionCtx) == 1);
  conditionExpr_ = std::dynamic_pointer_cast<Expression>(map.find(conditionCtx)->second);

  bool firstFlag = true;
  for (walangParser::TernaryExpressionBodyContext *body : ctx->ternaryExpressionBody()) {
    assert(map.count(body->expression()[0]) == 1);
    assert(map.count(body->expression()[1]) == 1);
    if (firstFlag) {
      this->leftExpr_ = std::dynamic_pointer_cast<Expression>(map.find(body->expression()[0])->second);
      this->rightExpr_ = std::dynamic_pointer_cast<Expression>(map.find(body->expression()[1])->second);
      firstFlag = false;
    } else {
      auto newRightExpr = std::make_shared<TernaryExpression>();
      newRightExpr->conditionExpr_ = this->rightExpr_;
      newRightExpr->leftExpr_ = std::dynamic_pointer_cast<Expression>(map.find(body->expression()[0])->second);
      newRightExpr->rightExpr_ = std::dynamic_pointer_cast<Expression>(map.find(body->expression()[1])->second);
      this->rightExpr_ = newRightExpr;
    }
  }
}

std::string TernaryExpression::to_string() const {
  return fmt::format("({0} ? {1} : {2})", conditionExpr_->to_string(), leftExpr_->to_string(), rightExpr_->to_string());
}

} // namespace ast
} // namespace walang