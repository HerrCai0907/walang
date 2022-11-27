#include "statement.hpp"
#include <fmt/core.h>

namespace walang {
namespace ast {

DeclareStatement::DeclareStatement(
    walangParser::DeclareStatementContext *ctx,
    std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map)
    : Statement(Statement::Type::_DeclareStatement) {
  name_ = ctx->Identifier()->getText();
  assert(map.count(ctx->expression()) == 1);
  initExpr_ = std::dynamic_pointer_cast<ast::Expression>(map.find(ctx->expression())->second);
}
std::string DeclareStatement::to_string() const {
  return fmt::format("declare '{0}' <- {1}\n", name_, initExpr_->to_string());
}

} // namespace ast
} // namespace walang