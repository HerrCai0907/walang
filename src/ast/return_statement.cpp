#include "expression.hpp"
#include "fmt/core.h"
#include "statement.hpp"
#include <cassert>

namespace walang::ast {

ReturnStatement::ReturnStatement(walangParser::ReturnStatementContext *ctx,
                                 std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map)
    : Statement(StatementType::TypeReturnStatement) {
  assert(map.count(ctx->expression()) == 1);
  expr_ = std::dynamic_pointer_cast<ast::Expression>(map.find(ctx->expression())->second);
}

[[nodiscard]] std::string ReturnStatement::to_string() const { return fmt::format("return {}\n", expr_->to_string()); }

} // namespace walang::ast