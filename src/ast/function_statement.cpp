#include "fmt/format.h"
#include "generated/walangParser.h"
#include "statement.hpp"
#include <algorithm>
#include <cassert>
#include <fmt/core.h>
#include <iterator>
#include <memory>
#include <vector>

namespace walang::ast {

FunctionStatement::FunctionStatement(walangParser::FunctionStatementContext *ctx,
                                     std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map)
    : Statement(StatementType::TypeFunctionStatement) {
  name_ = ctx->Identifier()->getText();

  auto parameterContexts = ctx->parameterList()->parameter();
  std::transform(parameterContexts.cbegin(), parameterContexts.cend(), std::back_inserter(arguments_),
                 [](walangParser::ParameterContext *parameterCtx) {
                   return Argument{parameterCtx->Identifier()->getText(), parameterCtx->type()->getText()};
                 });

  returnType_ = ctx->type() == nullptr ? std::nullopt : std::optional<std::string>{ctx->type()->getText()};

  assert(map.count(ctx->blockStatement()) == 1);
  body_ = std::dynamic_pointer_cast<BlockStatement>(map.find(ctx->blockStatement())->second);
}
std::string FunctionStatement::to_string() const {
  std::vector<std::string> argumentStrings{};
  std::transform(arguments_.cbegin(), arguments_.cend(), std::back_inserter(argumentStrings),
                 [](Argument const &argument) { return fmt::format("{0}:{1}", argument.name_, argument.type_); });
  return fmt::format("fn {0} ({1}) -> {2} {3}\n", name_, fmt::join(argumentStrings, ", "),
                     returnType_.value_or("__unknown__"), body_->to_string());
}

} // namespace walang