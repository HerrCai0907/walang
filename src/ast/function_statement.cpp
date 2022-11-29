#include "fmt/format.h"
#include "generated/walangParser.h"
#include "statement.hpp"
#include <algorithm>
#include <cassert>
#include <fmt/core.h>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace walang {
namespace ast {

FunctionStatement::FunctionStatement(walangParser::FunctionStatementContext *ctx,
                                     std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map)
    : Statement(Type::_FunctionStatement) {
  name_ = ctx->Identifier()->getText();

  auto parameterCtxs = ctx->parameterList()->parameter();
  std::transform(parameterCtxs.cbegin(), parameterCtxs.cend(), std::back_inserter(arguments_),
                 [](walangParser::ParameterContext *parameterCtx) {
                   return Argument{parameterCtx->Identifier()->getText(), parameterCtx->type()->getText()};
                 });

  returnType_ = ctx->type() == nullptr ? std::nullopt : std::optional<std::string>{ctx->type()->getText()};

  assert(map.count(ctx->blockStatement()) == 1);
  body_ = std::dynamic_pointer_cast<BlockStatement>(map.find(ctx->blockStatement())->second);
}
std::string FunctionStatement::to_string() const {
  std::vector<std::string> argumentStrs{};
  std::transform(arguments_.cbegin(), arguments_.cend(), std::back_inserter(argumentStrs),
                 [](Argument const &argument) { return fmt::format("{0}:{1}", argument.name_, argument.type_); });
  return fmt::format("fn {0} ({1}) -> {2} {3}\n", name_, fmt::join(argumentStrs, ", "),
                     returnType_.value_or("__unknown__"), body_->to_string());
}

} // namespace ast
} // namespace walang