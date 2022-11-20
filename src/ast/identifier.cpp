#include "expression.hpp"
#include "helper/overload.hpp"
#include <string>
#include <variant>

namespace walang {
namespace ast {

void Identifier::update(walangParser::IdentifierContext *ctx,
                        std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &) {
  if (ctx->Identifier() != nullptr) {
    id_ = ctx->getText();
  } else if (ctx->IntNumber() != nullptr) {
    id_ = std::stoull(ctx->getText());
  } else if (ctx->HexNumber() != nullptr) {
    id_ = std::stod(ctx->getText());
  }
}
std::string Identifier::to_string() const {
  return std::visit(overloaded{[](uint64_t i) { return "int:" + std::to_string(i); },
                               [](double d) { return "double:" + std::to_string(d); },
                               [](const std::string &s) { return "str:" + s; }},
                    id_);
}

} // namespace ast
} // namespace walang