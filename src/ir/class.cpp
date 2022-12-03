#include "binaryen-c.h"
#include "variant_type.hpp"
#include <memory>
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

} // namespace walang::ir