#include "statement.hpp"
#include <fmt/core.h>

namespace walang::ast {

DeclareStatement::DeclareStatement(walangParser::DeclareStatementContext *ctx,
                                   std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map)
    : Statement(StatementType::TypeDeclareStatement) {
  variantName_ = ctx->Identifier()->getText();
  if (ctx->type()) {
    variantType_ = ctx->type()->Identifier()->getText();
  }
  assert(map.count(ctx->expression()) == 1);
  initExpr_ = std::dynamic_pointer_cast<ast::Expression>(map.find(ctx->expression())->second);
}
std::string DeclareStatement::to_string() const {
  return fmt::format("declare {2}'{0}' <- {1}\n", variantName_, initExpr_->to_string(), variantType_);
}

} // namespace walang::ast