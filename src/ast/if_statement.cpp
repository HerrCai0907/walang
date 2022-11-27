#include "generated/walangParser.h"
#include "statement.hpp"
#include <cassert>
#include <fmt/core.h>
#include <fmt/format.h>
#include <iterator>
#include <memory>
#include <vector>

namespace walang {
namespace ast {

IfStatement::IfStatement(walangParser::IfStatementContext *ctx,
                         std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map)
    : Statement(Statement::Type::_IfStatement) {
  assert(map.count(ctx->expression()) == 1);
  condition_ = std::dynamic_pointer_cast<Expression>(map.find(ctx->expression())->second);
  auto blockStatements = ctx->blockStatement();
  assert(blockStatements.size() >= 1);
  assert(map.count(blockStatements.at(0)) == 1);
  thenBlock_ = std::dynamic_pointer_cast<BlockStatement>(map.find(blockStatements.at(0))->second);
  if (blockStatements.size() == 2U) {
    // if - then - else
    elseBlock_ = std::dynamic_pointer_cast<BlockStatement>(map.find(blockStatements.at(0))->second);
  } else if (ctx->ifStatement() != nullptr) {
    // if - then - else if ...
    elseBlock_ = std::dynamic_pointer_cast<IfStatement>(map.find(ctx->ifStatement())->second);
  } else {
    elseBlock_ = nullptr;
  }
}

std::string IfStatement::to_string() const {
  std::string elseStr;
  if (elseBlock_ != nullptr) {
    elseStr = fmt::format(" else {0}", elseBlock_->to_string());
  } else {
    elseStr = "";
  }
  return fmt::format("if {0} then {1}{2}", condition_->to_string(), thenBlock_->to_string(), elseStr);
}

} // namespace ast
} // namespace walang