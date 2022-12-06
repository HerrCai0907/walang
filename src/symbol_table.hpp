#pragma once

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "helper/diagnose.hpp"
#include <memory>
#include <string>
#include <vector>

namespace walang {

namespace ir {
class VariantType;
class Variant;
} // namespace ir

class VariantTypeMap {
public:
  VariantTypeMap();

  void registerType(std::string const &name, std::shared_ptr<ir::VariantType> const &type);
  std::shared_ptr<ir::VariantType> const &findVariantType(std::string const &name);

  std::shared_ptr<ir::VariantType> resolveType(std::string const &name);
  std::shared_ptr<ir::VariantType> inferType(std::shared_ptr<ast::Expression> const &initExpr,
                                             std::vector<std::shared_ptr<ir::Variant>> const &symbols);
  std::shared_ptr<ir::VariantType> getTypeFromDeclare(ast::DeclareStatement const &declare,
                                                      std::vector<std::shared_ptr<ir::Variant>> const &symbols);

private:
  std::map<std::string, std::shared_ptr<ir::VariantType>> map_{};

  void registerDefault();
};

} // namespace walang
