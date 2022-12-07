#pragma once

#include "ast/expression.hpp"
#include "helper/diagnose.hpp"
#include "ir/variant.hpp"
#include "variant_type_table.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace walang {

class Resolver {
public:
  explicit Resolver(std::shared_ptr<VariantTypeMap> variantTypeMap) : variantTypeMap_(std::move(variantTypeMap)) {}

  std::shared_ptr<ir::Symbol> resolveExpression(std::shared_ptr<ast::Expression> const &expression);
  std::shared_ptr<ir::Symbol> resolveIdentifier(std::shared_ptr<ast::Identifier> const &expression);
  std::shared_ptr<ir::Symbol> resolvePrefixExpression(std::shared_ptr<ast::PrefixExpression> const &expression);
  std::shared_ptr<ir::Symbol> resolveBinaryExpression(std::shared_ptr<ast::BinaryExpression> const &expression);
  std::shared_ptr<ir::Symbol> resolveTernaryExpression(std::shared_ptr<ast::TernaryExpression> const &expression);
  std::shared_ptr<ir::Symbol> resolveCallExpression(std::shared_ptr<ast::CallExpression> const &expression);
  std::shared_ptr<ir::Symbol> resolveMemberExpression(std::shared_ptr<ast::MemberExpression> const &expression);

  std::shared_ptr<ir::VariantType> resolveTypeExpression(std::shared_ptr<ast::Expression> const &expression);
  std::shared_ptr<ir::VariantType> resolveTypeIdentifier(std::shared_ptr<ast::Identifier> const &expression);
  std::shared_ptr<ir::VariantType>
  resolveTypePrefixExpression(std::shared_ptr<ast::PrefixExpression> const &expression);
  std::shared_ptr<ir::VariantType>
  resolveTypeBinaryExpression(std::shared_ptr<ast::BinaryExpression> const &expression);
  std::shared_ptr<ir::VariantType>
  resolveTypeTernaryExpression(std::shared_ptr<ast::TernaryExpression> const &expression);
  std::shared_ptr<ir::VariantType> resolveTypeCallExpression(std::shared_ptr<ast::CallExpression> const &expression);
  std::shared_ptr<ir::VariantType>
  resolveTypeMemberExpression(std::shared_ptr<ast::MemberExpression> const &expression);

  std::unordered_map<std::string, std::shared_ptr<ir::Global>> const &globals() { return globals_; }
  std::unordered_map<std::string, std::shared_ptr<ir::Function>> const &functions() { return functions_; }

  void setCurrentFunction(std::shared_ptr<ir::Function> currentFunction) {
    currentFunction_ = std::move(currentFunction);
  }
  void addGlobal(std::string const &name, std::shared_ptr<ir::Global> const &value) {
    auto it = globals_.emplace(name, value);
    if (!it.second) {
      throw RedefinedSymbol{name};
    }
  }
  void addFunction(std::string const &name, std::shared_ptr<ir::Function> const &value) {
    auto it = functions_.emplace(name, value);
    if (!it.second) {
      throw RedefinedSymbol{name};
    }
  }

private:
  std::shared_ptr<VariantTypeMap> variantTypeMap_{};
  std::unordered_map<std::string, std::shared_ptr<ir::Global>> globals_{};
  std::unordered_map<std::string, std::shared_ptr<ir::Function>> functions_{};
  std::shared_ptr<ir::Function> currentFunction_{};

  friend class Compiler; // TODO(fixme)
};

} // namespace walang