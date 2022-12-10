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

BinaryenExpressionRef Variant::assignTo(BinaryenModuleRef module, Variant const *to) const {
  switch (to->type()) {
  case Symbol::Type::TypeGlobal:
    return assignToGlobal(module, *dynamic_cast<Global const *>(to));
  case Symbol::Type::TypeLocal:
    return assignToLocal(module, *dynamic_cast<Local const *>(to));
  case Symbol::Type::TypeMemoryData:
    return assignToMemory(module, *dynamic_cast<MemoryData const *>(to));
  case Symbol::Type::TypeStackData:
    return assignToStack(module);
  case Symbol::Type::TypeFunction:
    break;
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
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