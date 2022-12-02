#pragma once

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "helper/diagnose.hpp"
#include <memory>
#include <string>

namespace walang {

namespace ir {
class VariantType;
}

class VariantTypeMap {
public:
  VariantTypeMap();

  void registerType(std::string const &name, std::shared_ptr<ir::VariantType> const &type);
  std::shared_ptr<ir::VariantType> const &findVariantType(std::string const &name);

  std::shared_ptr<ir::VariantType> const &resolveType(std::string const &name);
  std::shared_ptr<ir::VariantType> const &inferType(std::shared_ptr<ast::Expression> const &initExpr);
  std::shared_ptr<ir::VariantType> const &getTypeFromDeclare(ast::DeclareStatement const &declare);

private:
  std::map<std::string, std::shared_ptr<ir::VariantType>> map_{};

  void registerDefault();
};

} // namespace walang
