#include "binaryen-c.h"
#include "fmt/core.h"
#include "fmt/format.h"
#include "helper/diagnose.hpp"
#include "variant_type.hpp"
#include <algorithm>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>

namespace walang::ir {

Signature::Signature(std::vector<std::shared_ptr<VariantType>> const &argumentTypes,
                     std::shared_ptr<VariantType> const &returnType)
    : VariantType(Type::Signature), argumentTypes_(argumentTypes), returnType_(returnType) {}
std::string Signature::to_string() const {
  std::vector<std::string> argumentStr;
  std::transform(argumentTypes_.cbegin(), argumentTypes_.cend(), std::back_inserter(argumentStr),
                 [](std::shared_ptr<VariantType> const &argumentType) { return argumentType->to_string(); });
  return fmt::format("({0}) => {1}", fmt::join(argumentStr, ", "), returnType_->to_string());
}

BinaryenType Signature::underlyingTypeName() const { return BinaryenTypeInt32(); }

BinaryenExpressionRef Signature::handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                                BinaryenExpressionRef exprRef) const {
  throw InvalidOperator(shared_from_this(), op);
}
BinaryenExpressionRef Signature::handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                                BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                                std::shared_ptr<Function> const &function) {
  throw InvalidOperator(shared_from_this(), op);
}

} // namespace walang