#include "symbol_table.hpp"
#include "ast/expression.hpp"
#include "helper/diagnose.hpp"
#include "helper/overload.hpp"
#include "ir/variant.hpp"
#include "ir/variant_type.hpp"
#include <algorithm>
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

std::shared_ptr<ir::VariantType>
VariantTypeMap::getTypeFromDeclare(ast::DeclareStatement const &declare,
                                   std::vector<std::shared_ptr<ir::Variant>> const &symbols) {
  if (declare.variantType().empty()) {
    return inferType(declare.init(), symbols);
  } else {
    return resolveType(declare.variantType());
  }
}
std::shared_ptr<ir::VariantType> VariantTypeMap::resolveType(std::string const &typeName) {
  return findVariantType(typeName);
}
std::shared_ptr<ir::VariantType> VariantTypeMap::inferType(std::shared_ptr<ast::Expression> const &initExpr,
                                                           std::vector<std::shared_ptr<ir::Variant>> const &symbols) {
  if (initExpr->type() == ast::ExpressionType::TypeIdentifier) {
    auto identifier = std::dynamic_pointer_cast<ast::Identifier>(initExpr);
    return std::visit(overloaded{[this](uint64_t i) -> std::shared_ptr<ir::VariantType> { return resolveType("i32"); },
                                 [this](double d) -> std::shared_ptr<ir::VariantType> { return resolveType("f32"); },
                                 [&initExpr, &symbols](const std::string &s) -> std::shared_ptr<ir::VariantType> {
                                   auto it = std::find_if(symbols.begin(), symbols.end(),
                                                          [&s](std::shared_ptr<ir::Variant> const &variant) {
                                                            return variant->name() == s;
                                                          });
                                   if (it != symbols.end()) {
                                     return (*it)->variantType();
                                   }
                                   CannotInferType().setRangeAndThrow(initExpr->range());
                                 }},
                      identifier->identifier());
  }
  if (initExpr->type() == ast::ExpressionType::TypeCallExpression) {
    // `A();` which A is class name
    auto callExpr = std::dynamic_pointer_cast<ast::CallExpression>(initExpr);
    if (callExpr->caller()->type() == ast::ExpressionType::TypeIdentifier) {
      auto identifier = std::dynamic_pointer_cast<ast::Identifier>(callExpr->caller())->identifier();
      if (std::holds_alternative<std::string>(identifier)) {
        return resolveType(std::get<std::string>(identifier));
      }
    }
  }
  if (initExpr->type() == ast::ExpressionType::TypeMemberExpression) {
    // this.a
    auto memberExpr = std::dynamic_pointer_cast<ast::MemberExpression>(initExpr);
    auto exprType = inferType(memberExpr->expr(), symbols);
    if (exprType->type() == ir::VariantType::Type::Class) {
      auto members = std::dynamic_pointer_cast<ir::Class>(exprType)->member();
      auto it = std::find_if(members.begin(), members.end(), [&memberExpr](ir::Class::ClassMember const &member) {
        return member.memberName_ == memberExpr->member();
      });
      if (it != members.end()) {
        return it->memberType_;
      }
    }
  }
  CannotInferType().setRangeAndThrow(initExpr->range());
}

} // namespace walang