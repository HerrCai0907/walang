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
class Symbol;
} // namespace ir

class VariantTypeMap {
public:
  VariantTypeMap();

  void registerType(std::string const &name, std::shared_ptr<ir::VariantType> const &type);
  std::shared_ptr<ir::VariantType> findVariantType(std::string const &name);
  std::shared_ptr<ir::VariantType> tryFindVariantType(std::string const &name);

private:
  std::map<std::string, std::shared_ptr<ir::VariantType>> map_{};

  void registerDefault();
};

} // namespace walang
