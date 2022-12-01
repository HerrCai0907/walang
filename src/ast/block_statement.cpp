#include "generated/walangParser.h"
#include "statement.hpp"
#include <fmt/core.h>
#include <fmt/format.h>
#include <iterator>
#include <memory>
#include <vector>

namespace walang::ast {

BlockStatement::BlockStatement(walangParser::BlockStatementContext *ctx,
                               std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map)
    : Statement(StatementType::TypeBlockStatement) {
  auto statements = ctx->statement();
  std::transform(statements.cbegin(), statements.cend(), std::back_inserter(statements_),
                 [&map](walangParser::StatementContext *statementCtx) {
                   assert(map.count(statementCtx) == 1);
                   return std::dynamic_pointer_cast<Statement>(map.find(statementCtx)->second);
                 });
}

std::string BlockStatement::to_string() const {
  std::vector<std::string> statementStrings{};
  std::transform(statements_.cbegin(), statements_.cend(), std::back_inserter(statementStrings),
                 [](std::shared_ptr<Statement> const &statement) { return statement->to_string(); });
  return fmt::format("{{\n{0}}}", fmt::join(statementStrings, ""));
}

} // namespace walang::ast