#include "helper/diagnose.hpp"
#include "ir/variant.hpp"
#include "ir/variant_type.hpp"
#include <binaryen-c.h>
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace walang::ir {

Function::Function(std::string name, std::vector<std::string> const &argumentNames,
                   std::vector<std::shared_ptr<VariantType>> const &argumentTypes,
                   std::shared_ptr<VariantType> const &returnType)
    : Symbol(Type::TypeFunction, std::make_shared<Signature>(argumentTypes, returnType)), name_(std::move(name)),
      argumentSize_(argumentNames.size()) {
  assert(argumentNames.size() == argumentTypes.size());
  for (std::size_t i = 0; i < argumentSize_; i++) {
    addLocal(argumentNames[i], argumentTypes[i]);
  }
}

std::shared_ptr<Local> const &Function::addLocal(std::string const &name,
                                                 std::shared_ptr<VariantType> const &localType) {
  return locals_.emplace_back(std::make_shared<Local>(locals_.size(), name, localType));
}
std::shared_ptr<Local> const &Function::addTempLocal(std::shared_ptr<VariantType> const &localType) {
  return locals_.emplace_back(std::make_shared<Local>(locals_.size(), localType));
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
  std::vector<BinaryenType> binaryenTypes{};
  std::vector<std::string> localNames{};

  auto setTypeAndNameForLocals = [&binaryenTypes, &localNames](std::shared_ptr<Local> const &local) {
    const auto &variantType = local->variantType();
    if (variantType->type() == VariantType::Type::Class) {
      auto underlyingTypeNames = std::dynamic_pointer_cast<Class>(variantType)->underlyingTypeNames();
      binaryenTypes.insert(binaryenTypes.end(), underlyingTypeNames.begin(), underlyingTypeNames.end());
      for (uint32_t i = 0; i < underlyingTypeNames.size(); i++) {
        localNames.emplace_back(local->name() + "#" + std::to_string(i));
      }
    } else {
      binaryenTypes.push_back(variantType->underlyingTypeName());
      localNames.emplace_back(local->name());
    }
  };

  for (uint32_t i = 0; i < argumentSize_; i++) {
    setTypeAndNameForLocals(locals_[i]);
  }

  BinaryenType argumentBinaryenType = BinaryenTypeCreate(binaryenTypes.data(), binaryenTypes.size());
  binaryenTypes.clear();
  BinaryenType returnType = signature()->returnType()->underlyingTypeName();

  for (uint32_t i = argumentSize_; i < locals_.size(); i++) {
    setTypeAndNameForLocals(locals_[i]);
  }

  BinaryenFunctionRef funcRef = BinaryenAddFunction(module, name_.c_str(), argumentBinaryenType, returnType,
                                                    &binaryenTypes[0], binaryenTypes.size(), body);

  for (std::size_t i = 0; i < localNames.size(); i++) {
    if (localNames[i].empty()) {
      continue;
    }
    BinaryenFunctionSetLocalName(funcRef, i, localNames[i].c_str());
  }

  return funcRef;
}

} // namespace walang::ir