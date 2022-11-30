#include "function.hpp"
#include "binaryen-c.h"
#include "ir/variant.hpp"
#include "ir/variant_type.hpp"
#include <cassert>
#include <memory>

namespace walang {
namespace ir {

Function::Function(std::string const &name, std::vector<std::string> const &argumentNames,
                   std::vector<std::shared_ptr<VariantType>> const &argumentTypes,
                   std::shared_ptr<VariantType> const &returnType)
    : Symbol(Type::_Function, std::make_shared<Signature>(argumentTypes, returnType)), name_(name),
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
  if (currentBreakLabel_.size() == 0) {
    throw std::runtime_error("invalid break");
  }
  return currentBreakLabel_.top();
}
std::string const &Function::topContinueLabel() const {
  if (currentContinueLabel_.size() == 0) {
    throw std::runtime_error("invalid continue");
  }
  return currentContinueLabel_.top();
}
void Function::freeContinueLabel() { currentContinueLabel_.pop(); }
void Function::freeBreakLabel() { currentBreakLabel_.pop(); }

BinaryenFunctionRef Function::finalize(BinaryenModuleRef module, BinaryenExpressionRef body) {
  std::vector<BinaryenType> localBinaryenTypes{};
  std::transform(locals_.cbegin(), locals_.cend(), std::back_inserter(localBinaryenTypes),
                 [](std::shared_ptr<Local> const &local) { return local->variantType()->underlyingTypeName(); });
  BinaryenType argumentBinaryenType = BinaryenTypeCreate(&localBinaryenTypes[0], argumentSize_);
  BinaryenType returnType = signature()->returnType()->underlyingTypeName();
  BinaryenFunctionRef funcRef =
      BinaryenAddFunction(module, name_.c_str(), argumentBinaryenType, returnType, &localBinaryenTypes[argumentSize_],
                          localBinaryenTypes.size() - argumentSize_, body);

  for (std::size_t i = 0; i < locals_.size(); i++) {
    if (locals_[i]->name() == "") {
      continue;
    }
    BinaryenFunctionSetLocalName(funcRef, i, locals_[i]->name().c_str());
  }

  return funcRef;
}

} // namespace ir
} // namespace walang