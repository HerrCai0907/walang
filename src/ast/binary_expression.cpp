#include "expression.hpp"
#include <fmt/core.h>

namespace walang {
namespace ast {

BinaryExpression::BinaryExpression() noexcept : op_(static_cast<Op>(0)), leftExpr_(nullptr), rightExpr_(nullptr) {}
BinaryExpression::BinaryExpression(
    walangParser::BinaryExpressionContext *ctx,
    std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map) {
  assert(map.count(ctx->identifier()) == 1);
  leftExpr_ = std::dynamic_pointer_cast<Expression>(map.find(ctx->identifier())->second);
  std::vector<walangParser::BinaryExpressionRightContext *> binaryRights = ctx->binaryExpressionRight();
  bool firstRight = true;
  for (auto binaryRight : binaryRights) {
    Op op = Operator::getOp(binaryRight->binaryOperator());
    assert(map.count(binaryRight->expression()) == 1);
    auto rightExpr = std::dynamic_pointer_cast<Expression>(map.find(binaryRight->expression())->second);
    if (firstRight) {
      firstRight = false;
      op_ = op;
      rightExpr_ = rightExpr;
    } else {
      if (Operator::getOpPriority(op_) <= Operator::getOpPriority(op)) {
        // self as new operator's left
        auto newLeft = std::make_shared<BinaryExpression>();
        newLeft->op_ = this->op_;
        newLeft->leftExpr_ = this->leftExpr_;
        newLeft->rightExpr_ = this->rightExpr_;

        this->op_ = op;
        this->leftExpr_ = newLeft;
        this->rightExpr_ = rightExpr;
      } else {
        // combine new operator with right as new right
        auto newRight = std::make_shared<BinaryExpression>();
        newRight->op_ = op;
        newRight->leftExpr_ = this->rightExpr_;
        newRight->rightExpr_ = rightExpr;

        this->rightExpr_ = newRight;
      }
    }
  }
}

std::string BinaryExpression::to_string() const {
  return fmt::format("({0} {1} {2})", Operator::to_string(op_), leftExpr_->to_string(), rightExpr_->to_string());
}

} // namespace ast
} // namespace walang