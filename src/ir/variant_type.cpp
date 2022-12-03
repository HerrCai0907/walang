#include "variant_type.hpp"
#include "ast/expression.hpp"
#include "ast/op.hpp"
#include "helper/diagnose.hpp"
#include "helper/overload.hpp"
#include "symbol_table.hpp"
#include "variant.hpp"
#include <binaryen-c.h>
#include <cstdint>
#include <fmt/core.h>
#include <magic_enum.hpp>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

namespace walang::ir {

VariantType::VariantType(Type type) : type_(type) {}

std::string VariantType::to_string() const { return fmt::format("{}", magic_enum::enum_name(type_)); }

std::shared_ptr<VariantType> const &PendingResolveType::resolvedType() const {
  if (resolvedType_ == nullptr) {
    throw std::runtime_error("unresolved type '" + to_string() + "'");
  }
  return resolvedType_;
}

std::shared_ptr<VariantType> VariantType::from(BinaryenType underlyingType) {
  if (underlyingType == BinaryenTypeInt32()) {
    return std::make_shared<TypeI32>();
  } else if (underlyingType == BinaryenTypeInt64()) {
    return std::make_shared<TypeI64>();
  } else if (underlyingType == BinaryenTypeFloat32()) {
    return std::make_shared<TypeF32>();
  } else if (underlyingType == BinaryenTypeFloat64()) {
    return std::make_shared<TypeF64>();
  } else {
    throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
  }
}

BinaryenType PendingResolveType::underlyingTypeName() const { return resolvedType()->underlyingTypeName(); }
BinaryenType TypeNone::underlyingTypeName() const { return BinaryenTypeNone(); }
BinaryenType Int32::underlyingTypeName() const { return BinaryenTypeInt32(); }
BinaryenType Int64::underlyingTypeName() const { return BinaryenTypeInt64(); }
BinaryenType TypeF32::underlyingTypeName() const { return BinaryenTypeFloat32(); }
BinaryenType TypeF64::underlyingTypeName() const { return BinaryenTypeFloat64(); }

BinaryenExpressionRef VariantType::underlyingDefaultValue(BinaryenModuleRef module) const {
  auto typeName = underlyingTypeName();
  if (typeName == BinaryenTypeInt32()) {
    return BinaryenConst(module, BinaryenLiteralInt32(0));
  } else if (typeName == BinaryenTypeInt64()) {
    return BinaryenConst(module, BinaryenLiteralInt64(0));
  } else if (typeName == BinaryenTypeFloat32()) {
    return BinaryenConst(module, BinaryenLiteralFloat32(0));
  } else if (typeName == BinaryenTypeFloat64()) {
    return BinaryenConst(module, BinaryenLiteralFloat64(0));
  } else {
    throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
  }
}
BinaryenExpressionRef VariantType::underlyingConst(BinaryenModuleRef module, int64_t value) const {
  auto typeName = underlyingTypeName();
  if (typeName == BinaryenTypeInt32()) {
    return BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(value)));
  } else if (typeName == BinaryenTypeInt64()) {
    return BinaryenConst(module, BinaryenLiteralInt64(value));
  } else if (typeName == BinaryenTypeFloat32()) {
    return BinaryenConst(module, BinaryenLiteralFloat32(static_cast<float>(value)));
  } else if (typeName == BinaryenTypeFloat64()) {
    return BinaryenConst(module, BinaryenLiteralFloat64(static_cast<double>(value)));
  } else {
    throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
  }
}
BinaryenExpressionRef VariantType::underlyingConst(BinaryenModuleRef module, double value) const {
  if (underlyingTypeName() == BinaryenTypeInt32()) {
    throw TypeConvertError(to_string(), "i32");
  } else if (underlyingTypeName() == BinaryenTypeInt64()) {
    throw TypeConvertError(to_string(), "i64");
  } else if (underlyingTypeName() == BinaryenTypeFloat32()) {
    return BinaryenConst(module, BinaryenLiteralFloat32(static_cast<float>(value)));
  } else if (underlyingTypeName() == BinaryenTypeFloat64()) {
    return BinaryenConst(module, BinaryenLiteralFloat64(value));
  } else {
    throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
  }
}

bool VariantType::tryResolveTo(std::shared_ptr<VariantType> const &type) const {
  if (type->type_ == type_) {
    return true;
  }
  return false;
}
bool TypeAuto::tryResolveTo(std::shared_ptr<VariantType> const &type) const {
  resolvedType_ = type;
  return true;
}
bool TypeCondition::tryResolveTo(std::shared_ptr<VariantType> const &type) const {
  if (VariantType::tryResolveTo(type)) {
    return true;
  }
  if (type->type() == Type::I32 || type->type() == Type::U32 || type->type() == Type::I64 ||
      type->type() == Type::U64) {
    resolvedType_ = type;
    return true;
  }
  return false;
}

BinaryenExpressionRef PendingResolveType::handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                                         BinaryenExpressionRef exprRef) const {
  return resolvedType()->handlePrefixOp(module, op, exprRef);
}
BinaryenExpressionRef TypeNone::handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                               BinaryenExpressionRef exprRef) const {
  throw InvalidOperator(shared_from_this(), op);
}
BinaryenExpressionRef Int32::handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                            BinaryenExpressionRef exprRef) const {
  switch (op) {
  case ast::PrefixOp::ADD: {
    return exprRef;
  }
  case ast::PrefixOp::SUB: {
    BinaryenExpressionRef leftRef = underlyingConst(module, static_cast<int64_t>(0));
    return BinaryenBinary(module, BinaryenSubInt32(), leftRef, exprRef);
  }
  case ast::PrefixOp::NOT: {
    return BinaryenUnary(module, BinaryenEqZInt32(), exprRef);
  }
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef Int64::handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                            BinaryenExpressionRef exprRef) const {
  switch (op) {
  case ast::PrefixOp::ADD: {
    return exprRef;
  }
  case ast::PrefixOp::SUB: {
    BinaryenExpressionRef leftRef = underlyingConst(module, static_cast<int64_t>(0));
    return BinaryenBinary(module, BinaryenSubInt64(), leftRef, exprRef);
  }
  case ast::PrefixOp::NOT: {
    return BinaryenUnary(module, BinaryenEqZInt64(), exprRef);
  }
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef TypeF32::handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                              BinaryenExpressionRef exprRef) const {
  switch (op) {
  case ast::PrefixOp::ADD: {
    return exprRef;
  }
  case ast::PrefixOp::SUB: {
    BinaryenExpressionRef leftRef = underlyingConst(module, static_cast<double>(0));
    return BinaryenBinary(module, BinaryenSubFloat32(), leftRef, exprRef);
  }
  case ast::PrefixOp::NOT: {
    throw InvalidOperator(shared_from_this(), op);
  }
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef TypeF64::handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                              BinaryenExpressionRef exprRef) const {
  switch (op) {
  case ast::PrefixOp::ADD: {
    return exprRef;
  }
  case ast::PrefixOp::SUB: {
    BinaryenExpressionRef leftRef = underlyingConst(module, static_cast<double>(0));
    return BinaryenBinary(module, BinaryenSubFloat64(), leftRef, exprRef);
  }
  case ast::PrefixOp::NOT: {
    throw InvalidOperator(shared_from_this(), op);
  }
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}

BinaryenExpressionRef PendingResolveType::handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                                         BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                                         std::shared_ptr<Function> const &function) {
  return resolvedType()->handleBinaryOp(module, op, leftRef, rightRef, function);
}
BinaryenExpressionRef TypeNone::handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                               BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                               std::shared_ptr<Function> const &function) {
  throw InvalidOperator(shared_from_this(), op);
}
BinaryenExpressionRef TypeI32::handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                              BinaryenExpressionRef rightRef,
                                              std::shared_ptr<Function> const &function) {
  switch (op) {
  case ast::BinaryOp::ADD: {
    BinaryenOp op = BinaryenAddInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::SUB: {
    BinaryenOp op = BinaryenSubInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::MUL: {
    BinaryenOp op = BinaryenMulInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::DIV: {
    BinaryenOp op = BinaryenDivSInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::MOD: {
    BinaryenOp op = BinaryenRemSInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::LEFT_SHIFT: {
    BinaryenOp op = BinaryenShlInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::RIGHT_SHIFT: {
    BinaryenOp op = BinaryenShrSInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::LESS_THAN: {
    BinaryenOp op = BinaryenLtSInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::GREATER_THAN: {
    BinaryenOp op = BinaryenGtSInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NO_LESS_THAN: {
    BinaryenOp op = BinaryenGeSInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NO_GREATER_THAN: {
    BinaryenOp op = BinaryenLeSInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::EQUAL: {
    BinaryenOp op = BinaryenEqInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NOT_EQUAL: {
    BinaryenOp op = BinaryenNeInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::AND: {
    BinaryenOp op = BinaryenAndInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::OR: {
    BinaryenOp op = BinaryenOrInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::XOR: {
    BinaryenOp op = BinaryenXorInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::LOGIC_AND: {
    auto tempLocal = function->addTempLocal(shared_from_this());
    auto conditionalExprRef = BinaryenLocalTee(module, tempLocal->index(), leftRef, underlyingTypeName());
    auto loadLeftExprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, rightRef, loadLeftExprResult);
  }
  case ast::BinaryOp::LOGIC_OR: {
    auto tempLocal = function->addTempLocal(shared_from_this());
    auto conditionalExprRef = BinaryenLocalTee(module, tempLocal->index(), leftRef, underlyingTypeName());
    auto loadLeftExprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, loadLeftExprResult, rightRef);
  }
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef TypeU32::handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                              BinaryenExpressionRef rightRef,
                                              std::shared_ptr<Function> const &function) {
  switch (op) {
  case ast::BinaryOp::ADD: {
    BinaryenOp op = BinaryenAddInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::SUB: {
    BinaryenOp op = BinaryenSubInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::MUL: {
    BinaryenOp op = BinaryenMulInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::DIV: {
    BinaryenOp op = BinaryenDivUInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::MOD: {
    BinaryenOp op = BinaryenRemUInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::LEFT_SHIFT: {
    BinaryenOp op = BinaryenShlInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::RIGHT_SHIFT: {
    BinaryenOp op = BinaryenShrUInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::LESS_THAN: {
    BinaryenOp op = BinaryenLtUInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::GREATER_THAN: {
    BinaryenOp op = BinaryenGtUInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NO_LESS_THAN: {
    BinaryenOp op = BinaryenGeUInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NO_GREATER_THAN: {
    BinaryenOp op = BinaryenLeUInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::EQUAL: {
    BinaryenOp op = BinaryenEqInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NOT_EQUAL: {
    BinaryenOp op = BinaryenNeInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::AND: {
    BinaryenOp op = BinaryenAndInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::OR: {
    BinaryenOp op = BinaryenOrInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::XOR: {
    BinaryenOp op = BinaryenXorInt32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::LOGIC_AND: {
    auto tempLocal = function->addTempLocal(shared_from_this());
    auto conditionalExprRef = BinaryenLocalTee(module, tempLocal->index(), leftRef, underlyingTypeName());
    auto loadLeftExprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, rightRef, loadLeftExprResult);
  }
  case ast::BinaryOp::LOGIC_OR: {
    auto tempLocal = function->addTempLocal(shared_from_this());
    auto conditionalExprRef = BinaryenLocalTee(module, tempLocal->index(), leftRef, underlyingTypeName());
    auto loadLeftExprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, loadLeftExprResult, rightRef);
  }
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef TypeI64::handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                              BinaryenExpressionRef rightRef,
                                              std::shared_ptr<Function> const &function) {
  switch (op) {
  case ast::BinaryOp::ADD: {
    BinaryenOp op = BinaryenAddInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::SUB: {
    BinaryenOp op = BinaryenSubInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::MUL: {
    BinaryenOp op = BinaryenMulInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::DIV: {
    BinaryenOp op = BinaryenDivSInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::MOD: {
    BinaryenOp op = BinaryenRemSInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::LEFT_SHIFT: {
    BinaryenOp op = BinaryenShlInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::RIGHT_SHIFT: {
    BinaryenOp op = BinaryenShrSInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::LESS_THAN: {
    BinaryenOp op = BinaryenLtSInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::GREATER_THAN: {
    BinaryenOp op = BinaryenGtSInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NO_LESS_THAN: {
    BinaryenOp op = BinaryenGeSInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NO_GREATER_THAN: {
    BinaryenOp op = BinaryenLeSInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::EQUAL: {
    BinaryenOp op = BinaryenEqInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NOT_EQUAL: {
    BinaryenOp op = BinaryenNeInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::AND: {
    BinaryenOp op = BinaryenAndInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::OR: {
    BinaryenOp op = BinaryenOrInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::XOR: {
    BinaryenOp op = BinaryenXorInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::LOGIC_AND: {
    auto tempLocal = function->addTempLocal(shared_from_this());
    auto conditionalExprRef = BinaryenLocalTee(module, tempLocal->index(), leftRef, underlyingTypeName());
    auto loadLeftExprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, rightRef, loadLeftExprResult);
  }
  case ast::BinaryOp::LOGIC_OR: {
    auto tempLocal = function->addTempLocal(shared_from_this());
    auto conditionalExprRef = BinaryenLocalTee(module, tempLocal->index(), leftRef, underlyingTypeName());
    auto loadLeftExprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, loadLeftExprResult, rightRef);
  }
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef TypeU64::handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                              BinaryenExpressionRef rightRef,
                                              std::shared_ptr<Function> const &function) {
  switch (op) {
  case ast::BinaryOp::ADD: {
    BinaryenOp op = BinaryenAddInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::SUB: {
    BinaryenOp op = BinaryenSubInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::MUL: {
    BinaryenOp op = BinaryenMulInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::DIV: {
    BinaryenOp op = BinaryenDivUInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::MOD: {
    BinaryenOp op = BinaryenRemUInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::LEFT_SHIFT: {
    BinaryenOp op = BinaryenShlInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::RIGHT_SHIFT: {
    BinaryenOp op = BinaryenShrUInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::LESS_THAN: {
    BinaryenOp op = BinaryenLtUInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::GREATER_THAN: {
    BinaryenOp op = BinaryenGtUInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NO_LESS_THAN: {
    BinaryenOp op = BinaryenGeUInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NO_GREATER_THAN: {
    BinaryenOp op = BinaryenLeUInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::EQUAL: {
    BinaryenOp op = BinaryenEqInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NOT_EQUAL: {
    BinaryenOp op = BinaryenNeInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::AND: {
    BinaryenOp op = BinaryenAndInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::OR: {
    BinaryenOp op = BinaryenOrInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::XOR: {
    BinaryenOp op = BinaryenXorInt64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::LOGIC_AND: {
    auto tempLocal = function->addTempLocal(shared_from_this());
    auto conditionalExprRef = BinaryenLocalTee(module, tempLocal->index(), leftRef, underlyingTypeName());
    auto loadLeftExprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, rightRef, loadLeftExprResult);
  }
  case ast::BinaryOp::LOGIC_OR: {
    auto tempLocal = function->addTempLocal(shared_from_this());
    auto conditionalExprRef = BinaryenLocalTee(module, tempLocal->index(), leftRef, underlyingTypeName());
    auto loadLeftExprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, loadLeftExprResult, rightRef);
  }
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}

BinaryenExpressionRef TypeF32::handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                              BinaryenExpressionRef rightRef,
                                              std::shared_ptr<Function> const &function) {
  switch (op) {
  case ast::BinaryOp::ADD: {
    BinaryenOp op = BinaryenAddFloat32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::SUB: {
    BinaryenOp op = BinaryenSubFloat32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::MUL: {
    BinaryenOp op = BinaryenMulFloat32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::DIV: {
    BinaryenOp op = BinaryenDivFloat32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }

  case ast::BinaryOp::LESS_THAN: {
    BinaryenOp op = BinaryenLtFloat32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::GREATER_THAN: {
    BinaryenOp op = BinaryenGtFloat32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NO_LESS_THAN: {
    BinaryenOp op = BinaryenGeFloat32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NO_GREATER_THAN: {
    BinaryenOp op = BinaryenLeFloat32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::EQUAL: {
    BinaryenOp op = BinaryenEqFloat32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NOT_EQUAL: {
    BinaryenOp op = BinaryenNeFloat32();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::AND:
  case ast::BinaryOp::OR:
  case ast::BinaryOp::XOR:
  case ast::BinaryOp::MOD:
  case ast::BinaryOp::LEFT_SHIFT:
  case ast::BinaryOp::RIGHT_SHIFT:
  case ast::BinaryOp::LOGIC_AND:
  case ast::BinaryOp::LOGIC_OR: {
    throw InvalidOperator(shared_from_this(), op);
  }
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}

BinaryenExpressionRef TypeF64::handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                              BinaryenExpressionRef rightRef,
                                              std::shared_ptr<Function> const &function) {
  switch (op) {
  case ast::BinaryOp::ADD: {
    BinaryenOp op = BinaryenAddFloat64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::SUB: {
    BinaryenOp op = BinaryenSubFloat64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::MUL: {
    BinaryenOp op = BinaryenMulFloat64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::DIV: {
    BinaryenOp op = BinaryenDivFloat64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }

  case ast::BinaryOp::LESS_THAN: {
    BinaryenOp op = BinaryenLtFloat64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::GREATER_THAN: {
    BinaryenOp op = BinaryenGtFloat64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NO_LESS_THAN: {
    BinaryenOp op = BinaryenGeFloat64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NO_GREATER_THAN: {
    BinaryenOp op = BinaryenLeFloat64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::EQUAL: {
    BinaryenOp op = BinaryenEqFloat64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::NOT_EQUAL: {
    BinaryenOp op = BinaryenNeFloat64();
    return BinaryenBinary(module, op, leftRef, rightRef);
  }
  case ast::BinaryOp::AND:
  case ast::BinaryOp::OR:
  case ast::BinaryOp::XOR:
  case ast::BinaryOp::MOD:
  case ast::BinaryOp::LEFT_SHIFT:
  case ast::BinaryOp::RIGHT_SHIFT:
  case ast::BinaryOp::LOGIC_AND:
  case ast::BinaryOp::LOGIC_OR: {
    throw InvalidOperator(shared_from_this(), op);
  }
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}

} // namespace walang::ir