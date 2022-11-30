#include "expression.hpp"
#include "helper/overload.hpp"
#include <fmt/core.h>
#include <string>
#include <variant>

namespace walang {
namespace ast {

Identifier::Identifier(walangParser::IdentifierContext *ctx,
                       std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &)
    : Expression(Type::_Identifier) {
  if (ctx->Identifier() != nullptr) {
    id_ = ctx->getText();
  } else if (ctx->IntNumber() != nullptr) {
    id_ = std::stoull(ctx->getText());
  } else if (ctx->HexNumber() != nullptr) {
    id_ = std::stoull(ctx->getText(), nullptr, 16);
  } else if (ctx->FloatNumber() != nullptr) {
    id_ = std::stod(ctx->getText());
  } else {
    assert(false);
    std::abort();
  }
}
std::string Identifier::to_string() const {
  return std::visit(overloaded{[](uint64_t i) { return std::to_string(i); },
                               [](double d) { return fmt::format("{}", d); }, [](const std::string &s) { return s; }},
                    id_);
}

} // namespace ast
} // namespace walang