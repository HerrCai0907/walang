#include "variant.hpp"

namespace walang::ir {

void Local::initMembers(std::shared_ptr<VariantType> const &type) {
  if (type->type() == VariantType::Type::Class) {
    uint32_t index = index_;
    for (auto const &member : std::dynamic_pointer_cast<Class>(type)->member()) {
      members_.emplace(member.memberName_, std::make_shared<Local>(index, member.memberName_, member.memberType_));
      index++;
    }
  }
}
std::shared_ptr<Local> Local::findMemberByName(std::string const &name) const {
  auto it = members_.find(name);
  if (it == members_.end()) {
    return nullptr;
  }
  return it->second;
}

BinaryenExpressionRef Local::assignToMemory(BinaryenModuleRef module, MemoryData const &memoryData) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = variantType_->underlyingTypes();
  uint32_t offset = 0;
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto dataSize = VariantType::getSize(underlyingTypes[index]);
    auto loadExpr = BinaryenLocalGet(module, index_ + index, underlyingTypes[index]);
    auto storeExpr = BinaryenStore(
        module, dataSize, 0, 0,
        BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryData.memoryPosition() + offset))),
        loadExpr, underlyingTypes[index], "0");
    exprRefs.push_back(storeExpr);
    offset += dataSize;
  }
  return exprRefs.size() == 1 ? exprRefs.front()
                              : BinaryenBlock(module, nullptr, exprRefs.data(), exprRefs.size(), BinaryenTypeNone());
}
BinaryenExpressionRef Local::assignToLocal(BinaryenModuleRef module, Local const &local) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = variantType_->underlyingTypes();
  uint32_t offset = 0;
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto dataSize = VariantType::getSize(underlyingTypes[index]);
    auto loadExpr = BinaryenLocalGet(module, index_ + index, underlyingTypes[index]);
    auto storeExpr = BinaryenLocalSet(module, local.index() + index, loadExpr);
    exprRefs.push_back(storeExpr);
    offset += dataSize;
  }
  return exprRefs.size() == 1 ? exprRefs.front()
                              : BinaryenBlock(module, nullptr, exprRefs.data(), exprRefs.size(), BinaryenTypeNone());
}
BinaryenExpressionRef Local::assignToGlobal(BinaryenModuleRef module, Global const &global) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = variantType_->underlyingTypes();
  uint32_t offset = 0;
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto dataSize = VariantType::getSize(underlyingTypes[index]);
    auto loadExpr = BinaryenLocalGet(module, index_ + index, underlyingTypes[index]);
    auto globalName = underlyingTypes.size() == 1 ? global.name() : global.name() + "#" + std::to_string(index);
    auto storeExpr = BinaryenGlobalSet(module, globalName.c_str(), loadExpr);
    exprRefs.push_back(storeExpr);
    offset += dataSize;
  }
  return exprRefs.size() == 1 ? exprRefs.front()
                              : BinaryenBlock(module, nullptr, exprRefs.data(), exprRefs.size(), BinaryenTypeNone());
}
BinaryenExpressionRef Local::assignToStack(BinaryenModuleRef module) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = variantType_->underlyingTypes();
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto dataSize = VariantType::getSize(underlyingTypes[index]);
    auto loadExpr = BinaryenLocalGet(module, index_ + index, underlyingTypes[index]);
    exprRefs.push_back(loadExpr);
  }
  return exprRefs.size() == 1 ? exprRefs.front()
                              : BinaryenBlock(module, nullptr, exprRefs.data(), exprRefs.size(), BinaryenTypeNone());
}

} // namespace walang::ir