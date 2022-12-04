#include "binaryen-c.h"
#include "variant_type.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace walang::ir {

Class::Class(std::string className) : VariantType(Type::Class), className_(std::move(className)) {}

std::string Class::to_string() const { return className_; }

BinaryenType Class::underlyingType() const {
  std::vector<BinaryenType> binaryenTypes{};
  binaryenTypes.reserve(member_.size());
  for (auto const &member : member_) {
    binaryenTypes.push_back(member.memberType_->underlyingType());
  }
  return BinaryenTypeCreate(binaryenTypes.data(), binaryenTypes.size());
}
std::vector<BinaryenType> Class::underlyingTypes() const {
  std::vector<BinaryenType> binaryenTypes{};
  binaryenTypes.reserve(member_.size());
  for (auto const &member : member_) {
    auto memberUnderlyingTypeNames = member.memberType_->underlyingTypes();
    binaryenTypes.insert(binaryenTypes.end(), memberUnderlyingTypeNames.begin(), memberUnderlyingTypeNames.end());
  }
  return binaryenTypes;
}

BinaryenExpressionRef Class::handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                            BinaryenExpressionRef exprRef) const {
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef Class::handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                            BinaryenExpressionRef rightRef, std::shared_ptr<Function> const &function) {
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}

std::vector<BinaryenExpressionRef> Class::fromMemoryToLocal(BinaryenModuleRef module, uint32_t localBasisIndex,
                                                            uint32_t memoryPosition) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = this->underlyingTypes();
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto underlyingType = underlyingTypes[index];
    auto dataSize = VariantType::getSize(underlyingType);
    exprRefs.push_back(BinaryenLocalSet(
        module, localBasisIndex + index,
        BinaryenLoad(module, dataSize, false, 0, 0, underlyingType,
                     BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryPosition))), "0")));
    memoryPosition += dataSize;
  }
  return exprRefs;
}
std::vector<BinaryenExpressionRef> Class::fromLocalToMemory(BinaryenModuleRef module, uint32_t localBasisIndex,
                                                            uint32_t memoryPosition) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = this->underlyingTypes();
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto underlyingType = underlyingTypes[index];
    auto dataSize = VariantType::getSize(underlyingType);
    exprRefs.push_back(BinaryenStore(
        module, dataSize, 0, 0, BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryPosition))),
        BinaryenLocalGet(module, localBasisIndex + index, underlyingType), underlyingType, "0"));
    memoryPosition += dataSize;
  }
  return exprRefs;
}

static std::string getGlobalName(std::string const &globalName, uint32_t index, uint32_t totalSize) {
  if (totalSize == 1) {
    return globalName;
  } else {
    return globalName + "#" + std::to_string(index);
  }
}
std::vector<BinaryenExpressionRef> Class::fromMemoryToGlobal(BinaryenModuleRef module, std::string const &globalName,
                                                             uint32_t memoryPosition) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = this->underlyingTypes();
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto underlyingType = underlyingTypes[index];
    auto dataSize = VariantType::getSize(underlyingType);
    exprRefs.push_back(BinaryenGlobalSet(
        module, getGlobalName(globalName, index, underlyingTypes.size()).c_str(),
        BinaryenLoad(module, dataSize, false, 0, 0, underlyingType,
                     BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryPosition))), "0")));
    memoryPosition += dataSize;
  }
  return exprRefs;
}
std::vector<BinaryenExpressionRef> Class::fromGlobalToMemory(BinaryenModuleRef module, std::string const &globalName,
                                                             uint32_t memoryPosition) const {
  std::vector<BinaryenExpressionRef> exprRefs{};
  auto underlyingTypes = this->underlyingTypes();
  for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
    auto underlyingType = underlyingTypes[index];
    auto dataSize = VariantType::getSize(underlyingType);
    exprRefs.push_back(BinaryenStore(
        module, dataSize, 0, 0, BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryPosition))),
        BinaryenGlobalGet(module, getGlobalName(globalName, index, underlyingTypes.size()).c_str(), underlyingType),
        underlyingType, "0"));
    memoryPosition += dataSize;
  }
  return exprRefs;
}

} // namespace walang::ir