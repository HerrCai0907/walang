#include "expression.hpp"
#include <cassert>
#include <fmt/core.h>

namespace walang::ast {

BinaryExpression::BinaryExpression() noexcept
    : Expression(ExpressionType::TypeBinaryExpression), op_(static_cast<BinaryOp>(0)), leftExpr_(nullptr),
      rightExpr_(nullptr) {}
BinaryExpression::BinaryExpression(walangParser::BinaryExpressionContext *ctx,
                                   std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map)
    : Expression(ExpressionType::TypeBinaryExpression) {

  auto leftChild = dynamic_cast<antlr4::ParserRuleContext *>(ctx->binaryExpressionLeft()->children.at(0));
  assert(map.count(leftChild) == 1);
  leftExpr_ = std::dynamic_pointer_cast<Expression>(map.find(leftChild)->second);

  auto binaryRightWithOps = ctx->binaryExpressionRightWithOp();
  bool firstRight = true;
  for (auto binaryRightWithOp : binaryRightWithOps) {
    BinaryOp op = Operator::getOp(binaryRightWithOp->binaryOperator());
    auto rightCtx =
        dynamic_cast<antlr4::ParserRuleContext *>(binaryRightWithOp->binaryExpressionRight()->children.at(0));
    assert(map.count(rightCtx) == 1);
    auto rightExpr = std::dynamic_pointer_cast<Expression>(map.find(rightCtx)->second);
    if (firstRight) {
      firstRight = false;
      op_ = op;
      rightExpr_ = rightExpr;
    } else {
      appendExpr(op, rightExpr);
    }
  }
}
void BinaryExpression::appendExpr(BinaryOp op, const std::shared_ptr<Expression> &rightExpr) {
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
    if (std::dynamic_pointer_cast<BinaryExpression>(this->rightExpr_) != nullptr) {
      std::dynamic_pointer_cast<BinaryExpression>(this->rightExpr_)->appendExpr(op, rightExpr);
    } else {
      auto newRight = std::make_shared<BinaryExpression>();
      newRight->op_ = op;
      newRight->leftExpr_ = this->rightExpr_;
      newRight->rightExpr_ = rightExpr;
      this->rightExpr_ = newRight;
    }
  }
}

std::string BinaryExpression::to_string() const {
  return fmt::format("({0} {1} {2})", Operator::to_string(op_), leftExpr_->to_string(), rightExpr_->to_string());
}

} // namespace walang::ast