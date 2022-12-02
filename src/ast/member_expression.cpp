#include "expression.hpp"
#include <fmt/core.h>
#include <memory>
#include <utility>
#include <vector>

namespace walang::ast {

MemberExpression::MemberExpression() noexcept : Expression(ExpressionType::TypeMemberExpression) {}
MemberExpression::MemberExpression(std::shared_ptr<Expression> expr, walangParser::MemberExpressionRightContext *ctx)
    : Expression(ExpressionType::TypeMemberExpression), expr_(std::move(expr)) {
  member_ = ctx->Identifier()->getText();
}
MemberExpression::MemberExpression(walangParser::MemberExpressionContext *ctx,
                                   std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map)
    : Expression(ExpressionType::TypeMemberExpression) {
  auto callOrMemberExpressionLeft = ctx->callOrMemberExpressionLeft();
  if (callOrMemberExpressionLeft->identifier()) {
    assert(map.count(callOrMemberExpressionLeft->identifier()) == 1);
    this->expr_ = std::dynamic_pointer_cast<Expression>(map.find(callOrMemberExpressionLeft->identifier())->second);
  } else if (callOrMemberExpressionLeft->parenthesesExpression()) {
    assert(map.count(callOrMemberExpressionLeft->parenthesesExpression()) == 1);
    this->expr_ =
        std::dynamic_pointer_cast<Expression>(map.find(callOrMemberExpressionLeft->parenthesesExpression())->second);
  } else {
    throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
  }
  for (walangParser::CallOrMemberExpressionRightContext *rightCtx : ctx->callOrMemberExpressionRight()) {
    if (rightCtx->callExpressionRight()) {
      this->expr_ = std::make_shared<CallExpression>(this->expr_, rightCtx->callExpressionRight(), map);
    } else if (rightCtx->memberExpressionRight()) {
      this->expr_ = std::make_shared<MemberExpression>(this->expr_, rightCtx->memberExpressionRight());
    } else {
      throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
    }
  }
  member_ = ctx->memberExpressionRight()->Identifier()->getText();
}
std::string MemberExpression::to_string() const { return fmt::format("{0}.{1}", expr_->to_string(), member_); }

} // namespace walang::ast
