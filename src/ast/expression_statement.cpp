#include "statement.hpp"
#include <fmt/core.h>

namespace walang {
namespace ast {

ExpressionStatement::ExpressionStatement(
    walangParser::ExpressionStatementContext *ctx,
    std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map)
    : Statement(Statement::Type::ExpressionStatement) {
  assert(map.count(ctx->expression()) == 1);
  expr_ = std::dynamic_pointer_cast<ast::Expression>(map.find(ctx->expression())->second);
}
std::string ExpressionStatement::to_string() const { return fmt::format("{0}\n", expr_->to_string()); }

} // namespace ast
} // namespace walang