#include "binaryen/utils.hpp"
#include "variant.hpp"
#include <vector>

namespace walang::ir {

std::vector<BinaryenExpressionRef> MemoryData::assignToMemory(BinaryenModuleRef module,
                                                              MemoryData const &memoryData) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = variantType_->underlyingTypes();
  uint32_t offset = 0;
  for (unsigned long underlyingType : underlyingTypes) {
    auto bytes = VariantType::getSize(underlyingType);
    auto loadExpr =
        BinaryenLoad(module, bytes, false, 0, 0, underlyingType,
                     BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryPosition_ + offset))), "0");
    auto storeExpr = BinaryenStore(
        module, bytes, 0, 0,
        BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryData.memoryPosition() + offset))),
        loadExpr, underlyingType, "0");
    exprRefs.push_back(storeExpr);
    offset += bytes;
  }
  return exprRefs;
}
std::vector<BinaryenExpressionRef> MemoryData::assignToLocal(BinaryenModuleRef module, Local const &local) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = variantType_->underlyingTypes();
  uint32_t offset = 0;
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto bytes = VariantType::getSize(underlyingTypes[index]);
    auto loadExpr =
        BinaryenLoad(module, bytes, false, 0, 0, underlyingTypes[index],
                     BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryPosition_ + offset))), "0");
    auto storeExpr = BinaryenLocalSet(module, local.index() + index, loadExpr);
    exprRefs.push_back(storeExpr);
    offset += bytes;
  }
  return exprRefs;
}
std::vector<BinaryenExpressionRef> MemoryData::assignToGlobal(BinaryenModuleRef module, Global const &global) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = variantType_->underlyingTypes();
  uint32_t offset = 0;
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto bytes = VariantType::getSize(underlyingTypes[index]);
    auto loadExpr =
        BinaryenLoad(module, bytes, false, 0, 0, underlyingTypes[index],
                     BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryPosition_ + offset))), "0");
    auto globalName = underlyingTypes.size() == 1 ? global.name() : global.name() + "#" + std::to_string(index);
    auto storeExpr = BinaryenGlobalSet(module, globalName.c_str(), loadExpr);
    exprRefs.push_back(storeExpr);
    offset += bytes;
  }
  return exprRefs;
}
std::vector<BinaryenExpressionRef> MemoryData::assignToStack(BinaryenModuleRef module) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = variantType_->underlyingTypes();
  uint32_t offset = 0;
  for (BinaryenType underlyingType : underlyingTypes) {
    auto bytes = VariantType::getSize(underlyingType);
    auto loadExpr =
        BinaryenLoad(module, bytes, false, 0, 0, underlyingType,
                     BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryPosition_ + offset))), "0");
    exprRefs.push_back(loadExpr);
    offset += bytes;
  }
  return exprRefs;
}

} // namespace walang::ir
