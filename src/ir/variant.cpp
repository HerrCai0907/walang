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
  auto underlyingTypes = variantType_->underlyingTypes();
  switch (variantType_->underlyingReturnTypeStatus()) {
  case VariantType::UnderlyingReturnTypeStatus::None:
    return BinaryenNop(module);
  case VariantType::UnderlyingReturnTypeStatus::LoadFromMemory: {
    BinaryenIndex exprRefCount = BinaryenBlockGetNumChildren(exprRef);
    for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
      BinaryenIndex blockIndex = exprRefCount - underlyingTypes.size() + index;
      auto child = BinaryenBlockGetChildAt(exprRef, blockIndex);
      BinaryenBlockSetChildAt(exprRef, blockIndex, makeMultipleValueAssign(module, index, child));
      BinaryenExpressionSetType(exprRef, BinaryenTypeNone());
    }
    return exprRef;
  }
  case VariantType::UnderlyingReturnTypeStatus::ByReturnValue:
    return makeSingleValueAssign(module, exprRef);
  }
}

void Global::makeDefinition(BinaryenModuleRef module) {
  auto underlyingTypes = variantType_->underlyingTypes();
  if (underlyingTypes.size() == 1) {
    BinaryenAddGlobal(module, name_.c_str(), variantType_->underlyingType(), true,
                      variantType_->underlyingDefaultValue(module));
  } else {
    for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
      BinaryenAddGlobal(module, (name_ + "#" + std::to_string(index)).c_str(), underlyingTypes[index], true,
                        VariantType::from(underlyingTypes[index])->underlyingDefaultValue(module));
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
  return BinaryenGlobalGet(module, name_.c_str(), variantType_->underlyingType());
}

BinaryenExpressionRef Local::makeSingleValueAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) {
  return BinaryenLocalSet(module, index_, exprRef);
}
BinaryenExpressionRef Local::makeMultipleValueAssign(BinaryenModuleRef module, uint32_t index,
                                                     BinaryenExpressionRef exprRef) {
  return BinaryenLocalSet(module, index_ + index, exprRef);
}
BinaryenExpressionRef Local::makeGet(BinaryenModuleRef module) {
  return BinaryenLocalGet(module, index_, variantType_->underlyingType());
}

} // namespace walang::ir