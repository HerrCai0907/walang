#include "variant.hpp"
#include "ir/variant_type.hpp"
#include <binaryen-c.h>

#include <cassert>
#include <string>
#include <utility>

namespace walang::ir {

Global::Global(std::string name, std::shared_ptr<VariantType> const &type)
    : Variant(Type::TypeGlobal, type), name_{std::move(name)} {}
Local::Local(uint32_t index, std::shared_ptr<VariantType> const &type)
    : Variant(Type::TypeLocal, type), index_{index}, name_{} {}
Local::Local(uint32_t index, std::string name, std::shared_ptr<VariantType> const &type)
    : Variant(Type::TypeLocal, type), index_{index}, name_{std::move(name)} {}

BinaryenExpressionRef Variant::makeAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) {
  auto underlyingTypeNames = variantType_->underlyingTypeNames();
  if (underlyingTypeNames.size() == 1) {
    return makeSingleValueAssign(module, exprRef);
  } else {
    BinaryenIndex exprRefCount = BinaryenBlockGetNumChildren(exprRef);
    for (uint32_t index = 0; index < underlyingTypeNames.size(); index++) {
      BinaryenIndex blockIndex = exprRefCount - underlyingTypeNames.size() + index;
      auto child = BinaryenBlockGetChildAt(exprRef, blockIndex);
      BinaryenBlockSetChildAt(exprRef, blockIndex, makeMultipleValueAssign(module, index, child));
      BinaryenExpressionSetType(exprRef, BinaryenTypeNone());
    }
    return exprRef;
  }
}

void Global::makeDefinition(BinaryenModuleRef module) {
  auto underlyingTypeNames = variantType_->underlyingTypeNames();
  if (underlyingTypeNames.size() == 1) {
    BinaryenAddGlobal(module, name_.c_str(), variantType_->underlyingTypeName(), true,
                      variantType_->underlyingDefaultValue(module));
  } else {
    for (uint32_t index = 0; index < underlyingTypeNames.size(); index++) {
      BinaryenAddGlobal(module, (name_ + "#" + std::to_string(index)).c_str(), underlyingTypeNames[index], true,
                        VariantType::from(underlyingTypeNames[index])->underlyingDefaultValue(module));
    }
  }
}
BinaryenExpressionRef Global::makeSingleValueAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) {
  return BinaryenGlobalSet(module, name_.c_str(), exprRef);
}
BinaryenExpressionRef Global::makeMultipleValueAssign(BinaryenModuleRef module, uint32_t index,
                                                      BinaryenExpressionRef exprRef) {
  return BinaryenGlobalSet(module, (name_ + "#" + std::to_string(index)).c_str(), exprRef);
}
BinaryenExpressionRef Global::makeGet(BinaryenModuleRef module) {
  return BinaryenGlobalGet(module, name_.c_str(), variantType_->underlyingTypeName());
}

BinaryenExpressionRef Local::makeSingleValueAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) {
  return BinaryenLocalSet(module, index_, exprRef);
}
BinaryenExpressionRef Local::makeMultipleValueAssign(BinaryenModuleRef module, uint32_t index,
                                                     BinaryenExpressionRef exprRef) {
  return BinaryenLocalSet(module, index_ + index, exprRef);
}
BinaryenExpressionRef Local::makeGet(BinaryenModuleRef module) {
  return BinaryenLocalGet(module, index_, variantType_->underlyingTypeName());
}

} // namespace walang::ir