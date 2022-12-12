#include "binaryen/enum.hpp"
#include "variant.hpp"

namespace walang::ir {

BinaryenExpressionRef StackData::assignToMemory(BinaryenModuleRef module, MemoryData const &memoryData) const {
  auto underlyingTypes = variantType_->underlyingTypes();
  if (underlyingTypes.empty()) {
    return exprRef_;
  }
  if (underlyingTypes.size() == 1) {
    auto underlyingType = variantType_->underlyingType();
    auto bytes = VariantType::getSize(underlyingType);
    return BinaryenStore(module, bytes, 0, 0,
                         BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryData.memoryPosition()))),
                         exprRef_, underlyingType, "0");
  }
  assert(BinaryenExpressionGetId(exprRef_) == static_cast<uint32_t>(binaryen::Id::BlockId));
  assert(BinaryenBlockGetNumChildren(exprRef_) >= underlyingTypes.size());
  uint32_t offset = 0;
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    BinaryenIndex blockIndex = BinaryenBlockGetNumChildren(exprRef_) - underlyingTypes.size() + index;
    auto underlyingType = underlyingTypes[index];
    auto bytes = VariantType::getSize(underlyingType);
    auto valueExprRef = BinaryenBlockGetChildAt(exprRef_, blockIndex);
    auto setExprRef = BinaryenStore(
        module, bytes, 0, 0,
        BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryData.memoryPosition() + offset))),
        valueExprRef, underlyingType, "0");
    BinaryenBlockSetChildAt(exprRef_, blockIndex, setExprRef);
    offset += bytes;
  }
  BinaryenExpressionSetType(exprRef_, BinaryenTypeNone());
  return exprRef_;
}

BinaryenExpressionRef StackData::assignToLocal(BinaryenModuleRef module, Local const &local) const {
  auto underlyingTypes = variantType_->underlyingTypes();
  if (underlyingTypes.empty()) {
    return exprRef_;
  }
  if (underlyingTypes.size() == 1) {
    return BinaryenLocalSet(module, local.index(), exprRef_);
  }
  assert(BinaryenExpressionGetId(exprRef_) == static_cast<uint32_t>(binaryen::Id::BlockId));
  assert(BinaryenBlockGetNumChildren(exprRef_) == underlyingTypes.size());
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto valueExprRef = BinaryenBlockGetChildAt(exprRef_, index);
    auto setExprRef = BinaryenLocalSet(module, local.index() + index, valueExprRef);
    BinaryenBlockSetChildAt(exprRef_, index, setExprRef);
  }
  BinaryenExpressionSetType(exprRef_, BinaryenTypeNone());
  return exprRef_;
}

BinaryenExpressionRef StackData::assignToGlobal(BinaryenModuleRef module, Global const &global) const {
  auto underlyingTypes = variantType_->underlyingTypes();
  if (underlyingTypes.empty()) {
    return exprRef_;
  }
  if (underlyingTypes.size() == 1) {
    return BinaryenGlobalSet(module, global.name().c_str(), exprRef_);
  }
  assert(BinaryenExpressionGetId(exprRef_) == static_cast<uint32_t>(binaryen::Id::BlockId));
  assert(BinaryenBlockGetNumChildren(exprRef_) == underlyingTypes.size());
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto valueExprRef = BinaryenBlockGetChildAt(exprRef_, index);
    auto setExprRef = BinaryenGlobalSet(module, (global.name() + "#" + std::to_string(index)).c_str(), valueExprRef);
    BinaryenBlockSetChildAt(exprRef_, index, setExprRef);
  }
  BinaryenExpressionSetType(exprRef_, BinaryenTypeNone());
  return exprRef_;
}

} // namespace walang::ir