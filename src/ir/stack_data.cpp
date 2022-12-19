#include "binaryen-c.h"
#include "binaryen/utils.hpp"
#include "variant.hpp"
#include <vector>

namespace walang::ir {

std::vector<BinaryenExpressionRef> StackData::assignToMemory(BinaryenModuleRef module,
                                                             MemoryData const &memoryData) const {
  auto underlyingTypes = variantType_->underlyingTypes();
  if (underlyingTypes.empty()) {
    return exprRef_;
  }
  assert(exprRef_.size() >= underlyingTypes.size());
  auto result = exprRef_;
  uint32_t offset = 0;
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    BinaryenIndex blockIndex = exprRef_.size() - underlyingTypes.size() + index;
    auto underlyingType = underlyingTypes[index];
    auto bytes = VariantType::getSize(underlyingType);
    result[blockIndex] = BinaryenStore(
        module, bytes, 0, 0,
        BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryData.memoryPosition() + offset))),
        result[blockIndex], underlyingType, "0");

    offset += bytes;
  }
  return result;
}

std::vector<BinaryenExpressionRef> StackData::assignToLocal(BinaryenModuleRef module, Local const &local) const {
  auto underlyingTypes = variantType_->underlyingTypes();
  if (underlyingTypes.empty()) {
    return exprRef_;
  }
  assert(exprRef_.size() >= underlyingTypes.size());
  auto result = exprRef_;
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    BinaryenIndex blockIndex = exprRef_.size() - underlyingTypes.size() + index;
    result[blockIndex] = BinaryenLocalSet(module, local.index() + index, result[blockIndex]);
  }
  return result;
}

std::vector<BinaryenExpressionRef> StackData::assignToGlobal(BinaryenModuleRef module, Global const &global) const {
  auto underlyingTypes = variantType_->underlyingTypes();
  if (underlyingTypes.empty()) {
    return exprRef_;
  }
  assert(exprRef_.size() >= underlyingTypes.size());
  auto result = exprRef_;
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    BinaryenIndex blockIndex = exprRef_.size() - underlyingTypes.size() + index;
    auto globalName = underlyingTypes.size() == 1U ? global.name() : global.name() + "#" + std::to_string(index);
    result[blockIndex] = BinaryenGlobalSet(module, globalName.c_str(), result[blockIndex]);
  }
  return result;
}

} // namespace walang::ir