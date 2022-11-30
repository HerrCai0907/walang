#include "variant.hpp"
#include "ir/variant_type.hpp"
#include <binaryen-c.h>

namespace walang {
namespace ir {

Global::Global(std::string const &name, std::shared_ptr<VariantType> const &type)
    : Variant(Type::_Global, type), name_{name} {}
Local::Local(uint32_t index, std::shared_ptr<VariantType> const &type)
    : Variant(Type::_Local, type), index_{index}, name_{} {}
Local::Local(uint32_t index, std::string const &name, std::shared_ptr<VariantType> const &type)
    : Variant(Type::_Local, type), index_{index}, name_{name} {}

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

} // namespace ir
} // namespace walang