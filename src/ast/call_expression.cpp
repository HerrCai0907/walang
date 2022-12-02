#include "expression.hpp"
#include <algorithm>
#include <cassert>
#include <fmt/core.h>
#include <fmt/format.h>
#include <iostream>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

namespace walang::ast {

CallExpression::CallExpression() noexcept : Expression(ExpressionType::TypeCallExpression) {}
CallExpression::CallExpression(std::shared_ptr<Expression> caller, walangParser::CallExpressionRightContext *ctx,
                               std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map)
    : Expression(ExpressionType::TypeCallExpression), caller_(std::move(caller)) {
  for (auto exprCtx : ctx->expression()) {
    assert(map.count(exprCtx) == 1);
    this->arguments_.push_back(std::dynamic_pointer_cast<Expression>(map.find(exprCtx)->second));
  }
}
CallExpression::CallExpression(walangParser::CallExpressionContext *ctx,
                               std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map)
    : Expression(ExpressionType::TypeCallExpression) {
  auto callOrMemberExpressionLeft = ctx->callOrMemberExpressionLeft();
  if (callOrMemberExpressionLeft->identifier()) {
    assert(map.count(callOrMemberExpressionLeft->identifier()) == 1);
    this->caller_ = std::dynamic_pointer_cast<Expression>(map.find(callOrMemberExpressionLeft->identifier())->second);
  } else if (callOrMemberExpressionLeft->parenthesesExpression()) {
    assert(map.count(callOrMemberExpressionLeft->parenthesesExpression()) == 1);
    this->caller_ =
        std::dynamic_pointer_cast<Expression>(map.find(callOrMemberExpressionLeft->parenthesesExpression())->second);
  } else {
    throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
  }
  for (walangParser::CallOrMemberExpressionRightContext *rightCtx : ctx->callOrMemberExpressionRight()) {
    if (rightCtx->callExpressionRight()) {
      this->caller_ = std::make_shared<CallExpression>(this->caller_, rightCtx->callExpressionRight(), map);
    } else if (rightCtx->memberExpressionRight()) {
      this->caller_ = std::make_shared<MemberExpression>(this->caller_, rightCtx->memberExpressionRight());
    } else {
      throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
    }
  }
  for (auto exprCtx : ctx->callExpressionRight()->expression()) {
    assert(map.count(exprCtx) == 1);
    this->arguments_.push_back(std::dynamic_pointer_cast<Expression>(map.find(exprCtx)->second));
  }
}

std::string CallExpression::to_string() const {
  std::vector<std::string> argumentStrings{};
  std::transform(arguments_.cbegin(), arguments_.cend(), std::back_inserter(argumentStrings),
                 [](std::shared_ptr<Expression> const &expr) { return expr->to_string(); });
  return fmt::format("{0}({1})", caller_->to_string(), fmt::join(argumentStrings, ", "));
}

} // namespace walang::ast
