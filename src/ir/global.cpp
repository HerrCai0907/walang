#include "binaryen/utils.hpp"
#include "variant.hpp"

namespace walang::ir {

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

void Global::initMembers(std::shared_ptr<VariantType> const &type) {
  if (type->type() == VariantType::Type::Class) {
    for (auto const &member : std::dynamic_pointer_cast<Class>(type)->member()) {
      members_.emplace(member.memberName_, std::make_shared<Global>(member.memberName_, member.memberType_));
    }
  }
}
std::shared_ptr<Global> Global::findMemberByName(std::string const &name) const {
  auto it = members_.find(name);
  if (it == members_.end()) {
    return nullptr;
  }
  return it->second;
}

std::vector<BinaryenExpressionRef> Global::assignToMemory(BinaryenModuleRef module,
                                                          MemoryData const &memoryData) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = variantType_->underlyingTypes();
  uint32_t offset = 0;
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto dataSize = VariantType::getSize(underlyingTypes[index]);
    auto globalName = underlyingTypes.size() == 1 ? name_ : name_ + "#" + std::to_string(index);
    auto loadExpr = BinaryenGlobalGet(module, globalName.c_str(), underlyingTypes[index]);
    auto storeExpr = BinaryenStore(
        module, dataSize, 0, 0,
        BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryData.memoryPosition() + offset))),
        loadExpr, underlyingTypes[index], "0");
    exprRefs.push_back(storeExpr);
    offset += dataSize;
  }
  return exprRefs;
}
std::vector<BinaryenExpressionRef> Global::assignToLocal(BinaryenModuleRef module, Local const &local) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = variantType_->underlyingTypes();
  uint32_t offset = 0;
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto dataSize = VariantType::getSize(underlyingTypes[index]);
    auto globalName = underlyingTypes.size() == 1 ? name_ : name_ + "#" + std::to_string(index);
    auto loadExpr = BinaryenGlobalGet(module, globalName.c_str(), underlyingTypes[index]);
    auto storeExpr = BinaryenLocalSet(module, local.index() + index, loadExpr);
    exprRefs.push_back(storeExpr);
    offset += dataSize;
  }
  return exprRefs;
}
std::vector<BinaryenExpressionRef> Global::assignToGlobal(BinaryenModuleRef module, Global const &global) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = variantType_->underlyingTypes();
  uint32_t offset = 0;
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto dataSize = VariantType::getSize(underlyingTypes[index]);
    auto globalName = underlyingTypes.size() == 1 ? name_ : name_ + "#" + std::to_string(index);
    auto loadExpr = BinaryenGlobalGet(module, globalName.c_str(), underlyingTypes[index]);
    globalName = underlyingTypes.size() == 1 ? global.name() : global.name() + "#" + std::to_string(index);
    auto storeExpr = BinaryenGlobalSet(module, globalName.c_str(), loadExpr);
    exprRefs.push_back(storeExpr);
    offset += dataSize;
  }
  return exprRefs;
}
std::vector<BinaryenExpressionRef> Global::assignToStack(BinaryenModuleRef module) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = variantType_->underlyingTypes();
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto globalName = underlyingTypes.size() == 1 ? name_ : name_ + "#" + std::to_string(index);
    auto loadExpr = BinaryenGlobalGet(module, globalName.c_str(), underlyingTypes[index]);
    exprRefs.push_back(loadExpr);
  }
  return exprRefs;
}

} // namespace walang::ir