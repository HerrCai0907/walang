#include "expression.hpp"
#include <algorithm>
#include <cassert>
#include <fmt/core.h>
#include <fmt/format.h>
#include <iterator>
#include <memory>
#include <vector>

namespace walang::ast {

CallExpression::CallExpression() noexcept : Expression(ExpressionType::TypeCallExpression) {}
CallExpression::CallExpression(walangParser::CallExpressionContext *ctx,
                               std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map)
    : Expression(ExpressionType::TypeCallExpression) {
  bool first = true;
  assert(map.count(ctx->identifier()) == 1);
  for (walangParser::CallExpressionRightContext *rightCtx : ctx->callExpressionRight()) {
    if (first) {
      caller_ = std::dynamic_pointer_cast<Expression>(map.find(ctx->identifier())->second);
      first = false;
    } else {
      auto last = std::make_shared<CallExpression>();
      last->caller_ = caller_;
      last->arguments_ = arguments_;
      caller_ = last;
      arguments_.clear();
    }
    auto expressions = rightCtx->expression();
    std::transform(expressions.cbegin(), expressions.cend(), std::back_inserter(arguments_),
                   [&map](walangParser::ExpressionContext *exprCtx) {
                     assert(map.count(exprCtx) == 1);
                     return std::dynamic_pointer_cast<Expression>(map.find(exprCtx)->second);
                   });
  }
}

std::string CallExpression::to_string() const {
  std::vector<std::string> argumentStrings{};
  std::transform(arguments_.cbegin(), arguments_.cend(), std::back_inserter(argumentStrings),
                 [](std::shared_ptr<Expression> const &expr) { return expr->to_string(); });
  return fmt::format("{0}({1})", caller_->to_string(), fmt::join(argumentStrings, ", "));
}

} // namespace walang::ast
