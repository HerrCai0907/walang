#include "statement.hpp"
#include <fmt/core.h>

namespace walang {
namespace ast {

AssignStatement::AssignStatement(walangParser::AssignStatementContext *ctx,
                                 std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map)
    : Statement(Statement::Type::AssignStatement) {
  assert(map.count(ctx->expression(0)) == 1);
  assert(map.count(ctx->expression(1)) == 1);
  varExpr_ = std::dynamic_pointer_cast<ast::Expression>(map.find(ctx->expression(0))->second);
  valueExpr_ = std::dynamic_pointer_cast<ast::Expression>(map.find(ctx->expression(1))->second);
}
std::string AssignStatement::to_string() const {
  return fmt::format("{0} <- {1}\n", varExpr_->to_string(), valueExpr_->to_string());
}

} // namespace ast
} // namespace walang