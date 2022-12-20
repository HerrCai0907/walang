#include "resolver.hpp"
#include "helper/diagnose.hpp"
#include "helper/overload.hpp"
#include "ir/variant.hpp"
#include "ir/variant_type.hpp"
#include <algorithm>
#include <memory>

namespace walang {

std::shared_ptr<ir::Symbol> Resolver::resolveExpression(std::shared_ptr<ast::Expression> const &expression) {
  switch (expression->type()) {
  case ast::ExpressionType::TypeIdentifier:
    return resolveIdentifier(std::dynamic_pointer_cast<ast::Identifier>(expression));
  case ast::ExpressionType::TypePrefixExpression:
    return resolvePrefixExpression(std::dynamic_pointer_cast<ast::PrefixExpression>(expression));
  case ast::ExpressionType::TypeBinaryExpression:
    return resolveBinaryExpression(std::dynamic_pointer_cast<ast::BinaryExpression>(expression));
  case ast::ExpressionType::TypeTernaryExpression:
    return resolveTernaryExpression(std::dynamic_pointer_cast<ast::TernaryExpression>(expression));
  case ast::ExpressionType::TypeCallExpression:
    return resolveCallExpression(std::dynamic_pointer_cast<ast::CallExpression>(expression));
  case ast::ExpressionType::TypeMemberExpression:
    return resolveMemberExpression(std::dynamic_pointer_cast<ast::MemberExpression>(expression));
  }
  throw CannotResolveSymbol{};
}
std::shared_ptr<ir::Symbol> Resolver::resolveIdentifier(std::shared_ptr<ast::Identifier> const &expression) {
  return std::visit(overloaded{[&expression](uint64_t i) -> std::shared_ptr<ir::Symbol> {
                                 CannotResolveSymbol{}.setRangeAndThrow(expression->range());
                               },
                               [&expression](double d) -> std::shared_ptr<ir::Symbol> {
                                 CannotResolveSymbol{}.setRangeAndThrow(expression->range());
                               },
                               [&expression, this](const std::string &s) -> std::shared_ptr<ir::Symbol> {
                                 auto locals = currentFunction_->locals();
                                 auto localIt =
                                     std::find_if(locals.begin(), locals.end(),
                                                  [&s](std::shared_ptr<ir::Local> const &v) { return v->name() == s; });
                                 if (localIt != locals.end()) {
                                   return *localIt;
                                 }
                                 auto globalIt = globals_.find(s);
                                 if (globalIt != globals_.end()) {
                                   return globalIt->second;
                                 }
                                 auto functionIt = functions_.find(s);
                                 if (functionIt != functions_.end()) {
                                   return functionIt->second;
                                 }
                                 CannotResolveSymbol{}.setRangeAndThrow(expression->range());
                               }},
                    expression->identifier());
}

std::shared_ptr<ir::Symbol>
Resolver::resolvePrefixExpression(std::shared_ptr<ast::PrefixExpression> const &expression) {
  return resolveExpression(expression->expr());
}

std::shared_ptr<ir::Symbol>
Resolver::resolveBinaryExpression(std::shared_ptr<ast::BinaryExpression> const &expression) {
  return resolveExpression(expression->leftExpr()); // TODO(handle right expression)
}

std::shared_ptr<ir::Symbol>
Resolver::resolveTernaryExpression(std::shared_ptr<ast::TernaryExpression> const &expression) {
  return resolveExpression(expression->leftExpr()); // TODO(handle right expression)
}

std::shared_ptr<ir::Symbol> Resolver::resolveCallExpression(std::shared_ptr<ast::CallExpression> const &expression) {
  static_cast<void>(this);
  static_cast<void>(expression);
  throw CannotResolveSymbol{};
}

std::shared_ptr<ir::Symbol>
Resolver::resolveMemberExpression(std::shared_ptr<ast::MemberExpression> const &expression) {
  // this.a
  auto exprSymbol = resolveExpression(expression->expr());
  switch (exprSymbol->type()) {
  case ir::Symbol::Type::TypeLocal: {
    auto member = std::dynamic_pointer_cast<ir::Local>(exprSymbol)->findMemberByName(expression->member());
    if (member != nullptr) {
      return member;
    }
    auto methodMap = std::dynamic_pointer_cast<ir::Class>(exprSymbol->variantType())->methodMap();
    auto it = methodMap.find(expression->member());
    if (it != methodMap.cend()) {
      return it->second;
    }
    break;
  }
  case ir::Symbol::Type::TypeGlobal: {
    auto member = std::dynamic_pointer_cast<ir::Global>(exprSymbol)->findMemberByName(expression->member());
    if (member != nullptr) {
      return member;
    }
    auto methodMap = std::dynamic_pointer_cast<ir::Class>(exprSymbol->variantType())->methodMap();
    auto it = methodMap.find(expression->member());
    if (it != methodMap.cend()) {
      return it->second;
    }
    break;
  }
  case ir::Symbol::Type::TypeFunction:
  case ir::Symbol::Type::TypeMemoryData:
  case ir::Symbol::Type::TypeStackData:
    break;
  }
  throw CannotResolveSymbol{};
}

std::shared_ptr<ir::VariantType> Resolver::resolveTypeExpression(std::shared_ptr<ast::Expression> const &expression) {
  switch (expression->type()) {
  case ast::ExpressionType::TypeIdentifier:
    return resolveTypeIdentifier(std::dynamic_pointer_cast<ast::Identifier>(expression));
  case ast::ExpressionType::TypePrefixExpression:
    return resolveTypePrefixExpression(std::dynamic_pointer_cast<ast::PrefixExpression>(expression));
  case ast::ExpressionType::TypeBinaryExpression:
    return resolveTypeBinaryExpression(std::dynamic_pointer_cast<ast::BinaryExpression>(expression));
  case ast::ExpressionType::TypeTernaryExpression:
    return resolveTypeTernaryExpression(std::dynamic_pointer_cast<ast::TernaryExpression>(expression));
  case ast::ExpressionType::TypeCallExpression:
    return resolveTypeCallExpression(std::dynamic_pointer_cast<ast::CallExpression>(expression));
  case ast::ExpressionType::TypeMemberExpression:
    return resolveTypeMemberExpression(std::dynamic_pointer_cast<ast::MemberExpression>(expression));
  }
  throw CannotResolveSymbol{};
}
std::shared_ptr<ir::VariantType> Resolver::resolveTypeIdentifier(std::shared_ptr<ast::Identifier> const &expression) {
  return std::visit(
      overloaded{
          [this](uint64_t i) -> std::shared_ptr<ir::VariantType> { return variantTypeMap_->findVariantType("i32"); },
          [this](double d) -> std::shared_ptr<ir::VariantType> { return variantTypeMap_->findVariantType("f32"); },
          [&expression, this](const std::string &s) -> std::shared_ptr<ir::VariantType> {
            return resolveIdentifier(expression)->variantType();
          }},
      expression->identifier());
}
std::shared_ptr<ir::VariantType>
Resolver::resolveTypePrefixExpression(std::shared_ptr<ast::PrefixExpression> const &expression) {
  return resolveTypeExpression(expression->expr());
}
std::shared_ptr<ir::VariantType>
Resolver::resolveTypeBinaryExpression(std::shared_ptr<ast::BinaryExpression> const &expression) {
  return resolveTypeExpression(expression->leftExpr());
}
std::shared_ptr<ir::VariantType>
Resolver::resolveTypeTernaryExpression(std::shared_ptr<ast::TernaryExpression> const &expression) {
  return resolveTypeExpression(expression->leftExpr());
}
std::shared_ptr<ir::VariantType>
Resolver::resolveTypeCallExpression(std::shared_ptr<ast::CallExpression> const &expression) {
  auto callerSymbol = resolveExpression(expression->caller());
  switch (callerSymbol->type()) {
  case ir::Symbol::Type::TypeFunction:
    return std::dynamic_pointer_cast<ir::Function>(callerSymbol)->signature()->returnType();
  case ir::Symbol::Type::TypeGlobal:
  case ir::Symbol::Type::TypeLocal:
  case ir::Symbol::Type::TypeMemoryData:
  case ir::Symbol::Type::TypeStackData:
    break;
  }
  throw CannotResolveSymbol{};
}
std::shared_ptr<ir::VariantType>
Resolver::resolveTypeMemberExpression(std::shared_ptr<ast::MemberExpression> const &expression) {
  // this.a
  auto type = resolveTypeExpression(expression->expr());
  if (type->type() == ir::VariantType::Type::Class) {
    auto classMember = std::dynamic_pointer_cast<ir::Class>(type)->member();
    auto it = std::find_if(classMember.begin(), classMember.end(), [&expression](ir::Class::ClassMember const &member) {
      return member.memberName_ == expression->member();
    });
    if (it != classMember.end()) {
      return it->memberType_;
    }
  }
  throw CannotResolveSymbol{};
}

} // namespace walang
