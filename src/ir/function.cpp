#include "binaryen/utils.hpp"
#include "fmt/core.h"
#include "helper/diagnose.hpp"
#include "ir/variant.hpp"
#include "ir/variant_type.hpp"
#include <binaryen-c.h>
#include <cassert>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace walang::ir {

Function::Function(std::string name, std::vector<std::string> const &argumentNames,
                   std::vector<std::shared_ptr<VariantType>> const &argumentTypes,
                   std::shared_ptr<VariantType> const &returnType, std::set<Flag> const &flags,
                   BinaryenModuleRef module)
    : Symbol(Type::TypeFunction, std::make_shared<Signature>(argumentTypes, returnType)), name_(std::move(name)),
      argumentSize_(argumentNames.size()) {
  assert(argumentNames.size() == argumentTypes.size());
  // re-order arguments
  for (std::size_t i = 0; i < argumentSize_; i++) {
    addLocal(argumentNames[i], argumentTypes[i]);
  }
  if (flags.count(Flag::Method) == 1 && flags.count(Flag::Readonly) == 0) {
    assert(!locals_.empty() && "local should not be empty");
    // any change for `this` should be assigned back
    postExprRefs_ = locals_[locals_.size() - 1]->assignToMemory(
        module, MemoryData{ir::VariantType::getSize(returnType->underlyingType()), locals_[0]->variantType()});
  }
}

std::shared_ptr<Local> Function::addLocal(std::string const &name, std::shared_ptr<VariantType> const &localType) {
  if (findLocalByName(name) != nullptr) {
    throw RedefinedSymbol{name};
  }
  auto local = std::make_shared<Local>(localIndex_, name, localType);
  locals_.push_back(local);
  validLocals_.push_back(local);
  localIndex_ += localType->underlyingTypes().size();
  return local;
}
std::shared_ptr<Local> Function::addTempLocal(std::shared_ptr<VariantType> const &localType) {
  auto local = locals_.emplace_back(std::make_shared<Local>(localIndex_, localType));
  localIndex_ += localType->underlyingTypes().size();
  return local;
}
std::shared_ptr<Local> Function::findLocalByName(std::string const &name) const {
  auto it = std::find_if(validLocals_.cbegin(), validLocals_.cend(),
                         [&name](std::shared_ptr<Local> const &local) { return local->name() == name; });
  if (it != validLocals_.cend()) {
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

void Function::enterScope() noexcept { validLocalsIndex_.push(validLocals_.size()); }
void Function::exitScope() noexcept {
  validLocals_.resize(validLocalsIndex_.top());
  validLocalsIndex_.pop();
}

BinaryenFunctionRef Function::finalize(BinaryenModuleRef module, BinaryenExpressionRef body) {
  std::vector<BinaryenType> argumentBinaryenTypes{};
  std::vector<BinaryenType> localBinaryenTypes{};
  std::vector<std::string> localNames{};
  std::map<std::string, uint32_t> localNamesMap{};

  auto handleLocal = [&localNames, &localNamesMap](std::shared_ptr<Local> const &local,
                                                   std::vector<BinaryenType> &binaryenTypes) {
    std::string name = local->name();
    auto it = localNamesMap.find(name);
    if (it != localNamesMap.end()) {
      it->second++;
      name += "|" + std::to_string(it->second);
    } else {
      it = localNamesMap.insert(std::make_pair(name, 0U)).first;
    }
    auto underlyingTypes = local->variantType()->underlyingTypes();
    if (underlyingTypes.size() == 1) {
      binaryenTypes.push_back(local->variantType()->underlyingType());
      localNames.push_back(name);
    } else {
      for (uint32_t index = 0; index < underlyingTypes.size(); index++) {
        binaryenTypes.push_back(underlyingTypes[index]);
        localNames.push_back(name + "#" + std::to_string(index));
      }
    }
  };
  for (uint32_t i = 0; i < argumentSize_; i++) {
    handleLocal(locals_[i], argumentBinaryenTypes);
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
    handleLocal(locals_[i], localBinaryenTypes);
  }

  std::vector<BinaryenExpressionRef> bodyBlock{};
  bodyBlock.push_back(body);
  bodyBlock.insert(bodyBlock.end(), postExprRefs_.begin(), postExprRefs_.end());
  BinaryenExpressionRef bodyBlockRef = binaryen::Utils::combineExprRef(module, bodyBlock);
  BinaryenFunctionRef funcRef = BinaryenAddFunction(module, name_.c_str(), argumentBinaryenType, returnType,
                                                    localBinaryenTypes.data(), localBinaryenTypes.size(), bodyBlockRef);

  for (std::size_t i = 0; i < localNames.size(); i++) {
    if (!localNames[i].empty()) {
      BinaryenFunctionSetLocalName(funcRef, i, localNames[i].c_str());
    }
  }

  return funcRef;
}

std::vector<BinaryenExpressionRef> Function::finalizeReturn(BinaryenModuleRef module,
                                                            BinaryenExpressionRef returnExpr) {
  std::vector<BinaryenExpressionRef> ret{postExprRefs_.begin(), postExprRefs_.end()};
  ret.push_back(returnExpr);
  return ret;
}

void Function::checkArgumentAndReturnType(std::vector<std::shared_ptr<ast::Expression>> const &argumentExpressions,
                                          std::shared_ptr<ir::VariantType> const &expectedReturnType) const {
  if (argumentSize_ != argumentExpressions.size()) {
    auto e = ArgumentCountError(argumentSize_, argumentExpressions.size());
    throw e;
  }
  if (!expectedReturnType->tryResolveTo(signature()->returnType())) {
    auto e = TypeConvertError(signature()->returnType()->to_string(), expectedReturnType->to_string());
    throw e;
  }
}

} // namespace walang::ir