#include "variant_type_table.hpp"
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
std::shared_ptr<ir::VariantType> VariantTypeMap::findVariantType(std::string const &name) {
  auto ret = tryFindVariantType(name);
  if (ret == nullptr) {
    throw UnknownSymbol(name);
  }
  return ret;
}
std::shared_ptr<ir::VariantType> VariantTypeMap::tryFindVariantType(std::string const &name) {
  auto it = this->map_.find(name);
  if (it == map_.end()) {
    return nullptr;
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

} // namespace walang