#include "variant.hpp"
#include "ir/variant_type.hpp"
#include <binaryen-c.h>

#include <utility>

namespace walang::ir {

Global::Global(std::string name, std::shared_ptr<VariantType> const &type)
    : Variant(Type::TypeGlobal, type), name_{std::move(name)} {}
Local::Local(uint32_t index, std::shared_ptr<VariantType> const &type)
    : Variant(Type::TypeLocal, type), index_{index}, name_{} {}
Local::Local(uint32_t index, std::string name, std::shared_ptr<VariantType> const &type)
    : Variant(Type::TypeLocal, type), index_{index}, name_{std::move(name)} {}

BinaryenExpressionRef Global::makeAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) {
  return BinaryenGlobalSet(module, name_.c_str(), exprRef);
}
BinaryenExpressionRef Global::makeGet(BinaryenModuleRef module) {
  return BinaryenGlobalGet(module, name_.c_str(), variantType_->underlyingTypeName());
}
BinaryenExpressionRef Local::makeAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) {
  return BinaryenLocalSet(module, index_, exprRef);
}
BinaryenExpressionRef Local::makeGet(BinaryenModuleRef module) {
  return BinaryenLocalGet(module, index_, variantType_->underlyingTypeName());
}

} // namespace walang::ir