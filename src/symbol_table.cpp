#include "symbol_table.hpp"
#include "ast/expression.hpp"
#include "helper/overload.hpp"
#include "ir/variant_type.hpp"
#include <memory>
#include <string>
#include <variant>

namespace walang {

VariantTypeMap::VariantTypeMap() { registerDefault(); }
void VariantTypeMap::registerType(std::string const &name, std::shared_ptr<ir::VariantType> const &type) {
  auto ret = this->map_.try_emplace(name, type);
  if (!ret.second) {
    throw RedefinedSymbol(type->to_string());
  }
}
std::shared_ptr<ir::VariantType> const &VariantTypeMap::findVariantType(std::string const &name) {
  auto it = this->map_.find(name);
  if (it == map_.end()) {
    throw UnknownSymbol(name);
  }
  return it->second;
}
void VariantTypeMap::registerDefault() {
  registerType("i32", std::make_shared<ir::TypeI32>());
  registerType("u32", std::make_shared<ir::TypeU32>());
  registerType("i64", std::make_shared<ir::TypeI64>());
  registerType("u64", std::make_shared<ir::TypeU64>());
  registerType("f32", std::make_shared<ir::TypeF32>());
  registerType("f64", std::make_shared<ir::TypeF64>());
  registerType("void", std::make_shared<ir::TypeNone>());
}

std::shared_ptr<ir::VariantType> const &VariantTypeMap::getTypeFromDeclare(ast::DeclareStatement const &declare) {
  if (declare.variantType().empty()) {
    return inferType(declare.init());
  } else {
    return resolveType(declare.variantType());
  }
}
std::shared_ptr<ir::VariantType> const &VariantTypeMap::resolveType(std::string const &typeName) {
  return findVariantType(typeName);
}
std::shared_ptr<ir::VariantType> const &VariantTypeMap::inferType(std::shared_ptr<ast::Expression> const &initExpr) {
  if (initExpr->type() == ast::ExpressionType::TypeIdentifier) {
    auto identifier = std::dynamic_pointer_cast<ast::Identifier>(initExpr);
    return std::visit(
        overloaded{[this](uint64_t i) -> std::shared_ptr<ir::VariantType> const & { return resolveType("i32"); },
                   [this](double d) -> std::shared_ptr<ir::VariantType> const & { return resolveType("f32"); },
                   [](const std::string &s) -> std::shared_ptr<ir::VariantType> const & {
                     throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
                   }},
        identifier->identifier());
  }
  if (initExpr->type() == ast::ExpressionType::TypeCallExpression) {
    auto callExpr = std::dynamic_pointer_cast<ast::CallExpression>(initExpr);
    if (callExpr->caller()->type() == ast::ExpressionType::TypeIdentifier) {
      auto identifier = std::dynamic_pointer_cast<ast::Identifier>(callExpr->caller())->identifier();
      if (std::holds_alternative<std::string>(identifier)) {
        return resolveType(std::get<std::string>(identifier));
      }
    }
  }
  throw std::runtime_error("cannot infer type from " + initExpr->to_string());
}

} // namespace walang