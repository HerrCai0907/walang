#include "helper/diagnose.hpp"
#include "ir/variant.hpp"
#include "ir/variant_type.hpp"
#include <binaryen-c.h>
#include <cassert>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace walang::ir {

Function::Function(std::string name, std::vector<std::string> const &argumentNames,
                   std::vector<std::shared_ptr<VariantType>> const &argumentTypes,
                   std::shared_ptr<VariantType> const &returnType)
    : Symbol(Type::TypeFunction, std::make_shared<Signature>(argumentTypes, returnType)), name_(std::move(name)),
      argumentSize_(argumentNames.size()) {
  assert(argumentNames.size() == argumentTypes.size());
  // re-order arguments
  for (std::size_t i = 0; i < argumentSize_; i++) {
    if (argumentTypes[i]->type() != VariantType::Type::Class) {
      addLocal(argumentNames[i], argumentTypes[i]);
    }
  }
  for (std::size_t i = 0; i < argumentSize_; i++) {
    if (argumentTypes[i]->type() == VariantType::Type::Class) {
      addLocal(argumentNames[i], argumentTypes[i]);
    }
  }
}

std::shared_ptr<Local> Function::addLocal(std::string const &name, std::shared_ptr<VariantType> const &localType) {
  auto local = locals_.emplace_back(std::make_shared<Local>(localIndex_, name, localType));
  localIndex_ += localType->underlyingTypes().size();
  return local;
}
std::shared_ptr<Local> Function::addTempLocal(std::shared_ptr<VariantType> const &localType) {
  auto local = locals_.emplace_back(std::make_shared<Local>(localIndex_, localType));
  localIndex_ += localType->underlyingTypes().size();
  return local;
}
std::shared_ptr<Local> Function::findLocalByName(std::string const &name) const {
  auto it = std::find_if(locals_.cbegin(), locals_.cend(),
                         [&name](std::shared_ptr<Local> const &local) { return local->name() == name; });
  if (it != locals_.cend()) {
    return *it;
  }
  return nullptr;
}

std::string const &Function::createBreakLabel(std::string const &prefix) {
  std::string const &str = currentBreakLabel_.emplace(prefix + "|break|" + std::to_string(breakLabelIndex_));
  breakLabelIndex_++;
  return str;
}
std::string const &Function::createContinueLabel(std::string const &prefix) {
  std::string const &str = currentContinueLabel_.emplace(prefix + "|continue|" + std::to_string(continueLabelIndex_));
  continueLabelIndex_++;
  return str;
}
std::string const &Function::topBreakLabel() const {
  if (currentBreakLabel_.empty()) {
    throw JumpStatementError("invalid break");
  }
  return currentBreakLabel_.top();
}
std::string const &Function::topContinueLabel() const {
  if (currentContinueLabel_.empty()) {
    throw JumpStatementError("invalid continue");
  }
  return currentContinueLabel_.top();
}
void Function::freeContinueLabel() { currentContinueLabel_.pop(); }
void Function::freeBreakLabel() { currentBreakLabel_.pop(); }

BinaryenFunctionRef Function::finalize(BinaryenModuleRef module, BinaryenExpressionRef body) {
  std::vector<BinaryenType> argumentBinaryenTypes{};
  std::vector<std::string> argumentNames{};
  std::vector<BinaryenType> localBinaryenTypes{};
  std::vector<std::string> localNames{};

  std::deque<BinaryenExpressionRef> bodyWithMemoryOperatorDeque{body};

  auto handleLocal = [](std::shared_ptr<Local> const &local, std::vector<BinaryenType> &binaryenTypes,
                        std::vector<std::string> &names) {
    auto underlyingTypes = local->variantType()->underlyingTypes();
    if (underlyingTypes.size() == 1) {
      binaryenTypes.push_back(local->variantType()->underlyingType());
      names.emplace_back(local->name());
    } else {
      for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
        binaryenTypes.push_back(underlyingTypes[index]);
        names.emplace_back(local->name() + "#" + std::to_string(index));
      }
    }
  };

  uint32_t memoryPosition = 0;
  auto handlePassValueByMemory = [&module, &memoryPosition,
                                  &bodyWithMemoryOperatorDeque](std::shared_ptr<Local> const &local) {
    auto underlyingTypes = local->variantType()->underlyingTypes();
    for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
      auto underlyingType = underlyingTypes[index];
      auto dataSize = VariantType::getSize(underlyingType);
      bodyWithMemoryOperatorDeque.push_front(BinaryenLocalSet(
          module, local->index() + index,
          BinaryenLoad(module, dataSize, false, 0, 0, underlyingType,
                       BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryPosition))), "0")));
      bodyWithMemoryOperatorDeque.push_back(BinaryenStore(
          module, dataSize, 0, 0, BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(memoryPosition))),
          BinaryenLocalGet(module, local->index() + index, underlyingType), underlyingType, "0"));
      memoryPosition += dataSize;
    }
  };
  for (uint32_t i = 0; i < argumentSize_; i++) {
    if (locals_[i]->variantType()->type() == VariantType::Type::Class) {
      handlePassValueByMemory(locals_[i]);
      handleLocal(locals_[i], localBinaryenTypes, localNames);
    } else {
      handleLocal(locals_[i], argumentBinaryenTypes, argumentNames);
    }
  }
  BinaryenType argumentBinaryenType = BinaryenTypeCreate(argumentBinaryenTypes.data(), argumentBinaryenTypes.size());

  BinaryenType returnType;
  switch (signature()->returnType()->underlyingReturnTypeStatus()) {
  case VariantType::UnderlyingReturnTypeStatus::None:
  case VariantType::UnderlyingReturnTypeStatus::LoadFromMemory:
    returnType = BinaryenTypeNone();
    break;
  case VariantType::UnderlyingReturnTypeStatus::ByReturnValue:
    returnType = signature()->returnType()->underlyingType();
    break;
  }
  for (uint32_t i = argumentSize_; i < locals_.size(); i++) {
    handleLocal(locals_[i], localBinaryenTypes, localNames);
  }

  std::vector<BinaryenExpressionRef> bodyWithMemoryOperatorVec{bodyWithMemoryOperatorDeque.cbegin(),
                                                               bodyWithMemoryOperatorDeque.cend()};
  BinaryenExpressionRef bodyWithMemoryOperatorRef =
      bodyWithMemoryOperatorVec.size() > 1 ? BinaryenBlock(module, nullptr, bodyWithMemoryOperatorVec.data(),
                                                           bodyWithMemoryOperatorVec.size(), BinaryenTypeAuto())
                                           : body;
  BinaryenFunctionRef funcRef =
      BinaryenAddFunction(module, name_.c_str(), argumentBinaryenType, returnType, localBinaryenTypes.data(),
                          localBinaryenTypes.size(), bodyWithMemoryOperatorRef);

  std::size_t i = 0;
  for (; i < argumentNames.size(); i++) {
    if (argumentNames[i].empty()) {
      continue;
    }
    BinaryenFunctionSetLocalName(funcRef, i, argumentNames[i].c_str());
  }
  for (; i < localNames.size() + argumentNames.size(); i++) {
    if (localNames[i - argumentNames.size()].empty()) {
      continue;
    }
    BinaryenFunctionSetLocalName(funcRef, i, localNames[i - argumentNames.size()].c_str());
  }

  return funcRef;
}

} // namespace walang::ir