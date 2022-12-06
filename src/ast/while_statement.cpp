#include "generated/walangParser.h"
#include "statement.hpp"
#include <cassert>
#include <fmt/core.h>
#include <memory>

namespace walang::ast {

WhileStatement::WhileStatement(walangParser::WhileStatementContext *ctx,
                               std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map)
    : Statement(StatementType::TypeWhileStatement) {
  assert(map.count(ctx->expression()) == 1);
  condition_ = std::dynamic_pointer_cast<Expression>(map.find(ctx->expression())->second);
  assert(map.count(ctx->blockStatement()) == 1);
  block_ = std::dynamic_pointer_cast<BlockStatement>(map.find(ctx->blockStatement())->second);
}

std::string WhileStatement::to_string() const {
  return fmt::format("while {0} {1}", condition_->to_string(), block_->to_string());
}

} // namespace walang::ast