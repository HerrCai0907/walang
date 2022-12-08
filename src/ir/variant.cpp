#include "variant.hpp"
#include "binaryen/enum.hpp"
#include "helper/diagnose.hpp"
#include "ir/variant_type.hpp"
#include <binaryen-c.h>
#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace walang::ir {

Global::Global(std::string name, std::shared_ptr<VariantType> const &type)
    : Variant(std::move(name), Type::TypeGlobal, type) {}
Local::Local(uint32_t index, std::shared_ptr<VariantType> const &type)
    : Variant("", Type::TypeLocal, type), index_{index} {
  initMembers(type);
}
Local::Local(uint32_t index, std::string name, std::shared_ptr<VariantType> const &type)
    : Variant(std::move(name), Type::TypeLocal, type), index_{index} {
  initMembers(type);
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
    throw UnknownSymbol(name);
  }
  return it->second;
}

BinaryenExpressionRef Global::assignToMemory(BinaryenModuleRef module, MemoryData const &memoryData) const {
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
  return exprRefs.size() == 1 ? exprRefs.front()
                              : BinaryenBlock(module, nullptr, exprRefs.data(), exprRefs.size(), BinaryenTypeNone());
}
BinaryenExpressionRef Global::assignToLocal(BinaryenModuleRef module, Local const &local) const {
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
  return exprRefs.size() == 1 ? exprRefs.front()
                              : BinaryenBlock(module, nullptr, exprRefs.data(), exprRefs.size(), BinaryenTypeNone());
}
BinaryenExpressionRef Global::assignToGlobal(BinaryenModuleRef module, Global const &global) const {
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
  return exprRefs.size() == 1 ? exprRefs.front()
                              : BinaryenBlock(module, nullptr, exprRefs.data(), exprRefs.size(), BinaryenTypeNone());
}
BinaryenExpressionRef Global::assignToStack(BinaryenModuleRef module) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = variantType_->underlyingTypes();
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto globalName = underlyingTypes.size() == 1 ? name_ : name_ + "#" + std::to_string(index);
    auto loadExpr = BinaryenGlobalGet(module, globalName.c_str(), underlyingTypes[index]);
    exprRefs.push_back(loadExpr);
  }
  return exprRefs.size() == 1 ? exprRefs.front()
                              : BinaryenBlock(module, nullptr, exprRefs.data(), exprRefs.size(), BinaryenTypeNone());
}
BinaryenExpressionRef Global::assignFromStack(BinaryenModuleRef module, BinaryenExpressionRef exprRef) const {
  auto underlyingTypes = variantType_->underlyingTypes();
  if (underlyingTypes.empty()) {
    return exprRef;
  }
  if (underlyingTypes.size() == 1) {
    return BinaryenGlobalSet(module, name_.c_str(), exprRef);
  }
  assert(BinaryenExpressionGetId(exprRef) == static_cast<uint32_t>(binaryen::Id::BlockId));
  assert(BinaryenBlockGetNumChildren(exprRef) == underlyingTypes.size());
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto valueExprRef = BinaryenBlockGetChildAt(exprRef, index);
    auto setExprRef = BinaryenGlobalSet(module, (name_ + "#" + std::to_string(index)).c_str(), valueExprRef);
    BinaryenBlockSetChildAt(exprRef, index, setExprRef);
  }
  BinaryenExpressionSetType(exprRef, BinaryenTypeNone());
  return exprRef;
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
BinaryenExpressionRef Local::assignFromStack(BinaryenModuleRef module, BinaryenExpressionRef exprRef) const {
  auto underlyingTypes = variantType_->underlyingTypes();
  if (underlyingTypes.empty()) {
    return exprRef;
  }
  if (underlyingTypes.size() == 1) {
    return BinaryenLocalSet(module, index_, exprRef);
  }
  assert(BinaryenExpressionGetId(exprRef) == static_cast<uint32_t>(binaryen::Id::BlockId));
  assert(BinaryenBlockGetNumChildren(exprRef) == underlyingTypes.size());
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto valueExprRef = BinaryenBlockGetChildAt(exprRef, index);
    auto setExprRef = BinaryenLocalSet(module, index_ + index, valueExprRef);
    BinaryenBlockSetChildAt(exprRef, index, setExprRef);
  }
  BinaryenExpressionSetType(exprRef, BinaryenTypeNone());
  return exprRef;
}

BinaryenExpressionRef MemoryData::assignToMemory(BinaryenModuleRef module, MemoryData const &memoryData) const {
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
  return exprRefs.size() == 1 ? exprRefs.front()
                              : BinaryenBlock(module, nullptr, exprRefs.data(), exprRefs.size(), BinaryenTypeNone());
}
BinaryenExpressionRef MemoryData::assignToLocal(BinaryenModuleRef module, Local const &local) const {
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
  return exprRefs.size() == 1 ? exprRefs.front()
                              : BinaryenBlock(module, nullptr, exprRefs.data(), exprRefs.size(), BinaryenTypeNone());
}
BinaryenExpressionRef MemoryData::assignToGlobal(BinaryenModuleRef module, Global const &global) const {
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
  return exprRefs.size() == 1 ? exprRefs.front()
                              : BinaryenBlock(module, nullptr, exprRefs.data(), exprRefs.size(), BinaryenTypeNone());
}
BinaryenExpressionRef MemoryData::assignToStack(BinaryenModuleRef module) const {
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
  return exprRefs.size() == 1 ? exprRefs.front()
                              : BinaryenBlock(module, nullptr, exprRefs.data(), exprRefs.size(), BinaryenTypeNone());
}
BinaryenExpressionRef MemoryData::assignFromStack(BinaryenModuleRef module, BinaryenExpressionRef exprRef) const {
  auto underlyingTypes = variantType_->underlyingTypes();
  if (underlyingTypes.empty()) {
    return exprRef;
  }
  if (underlyingTypes.size() == 1) {
    auto underlyingType = variantType_->underlyingType();
    auto bytes = VariantType::getSize(underlyingType);
    return BinaryenStore(module, bytes, 0, 0,
                         BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryPosition_))), exprRef,
                         underlyingType, "0");
  }
  assert(BinaryenExpressionGetId(exprRef) == static_cast<uint32_t>(binaryen::Id::BlockId));
  assert(BinaryenBlockGetNumChildren(exprRef) == underlyingTypes.size());
  uint32_t offset = 0;
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto underlyingType = underlyingTypes[index];
    auto bytes = VariantType::getSize(underlyingType);
    auto valueExprRef = BinaryenBlockGetChildAt(exprRef, index);
    auto setExprRef =
        BinaryenStore(module, bytes, 0, 0,
                      BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryPosition_ + offset))),
                      exprRef, underlyingType, "0");
    BinaryenBlockSetChildAt(exprRef, index, setExprRef);
    offset += bytes;
  }
  BinaryenExpressionSetType(exprRef, BinaryenTypeNone());
  return exprRef;
}

} // namespace walang::ir