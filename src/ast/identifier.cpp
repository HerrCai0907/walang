#include "expression.hpp"
#include "helper/overload.hpp"
#include <fmt/core.h>
#include <string>
#include <variant>

namespace walang::ast {

Identifier::Identifier(walangParser::IdentifierContext *ctx,
                       std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &)
    : Expression(ExpressionType::TypeIdentifier) {
  if (ctx->Identifier() != nullptr) {
    identifier_ = ctx->getText();
  } else if (ctx->IntNumber() != nullptr) {
    identifier_ = std::stoull(ctx->getText());
  } else if (ctx->HexNumber() != nullptr) {
    identifier_ = std::stoull(ctx->getText(), nullptr, 16);
  } else if (ctx->FloatNumber() != nullptr) {
    identifier_ = std::stod(ctx->getText());
  } else {
    throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
  }
}
std::string Identifier::to_string() const {
  return std::visit(overloaded{[](uint64_t i) { return std::to_string(i); },
                               [](double d) { return fmt::format("{}", d); }, [](const std::string &s) { return s; }},
                    identifier_);
}

} // namespace walang::ast