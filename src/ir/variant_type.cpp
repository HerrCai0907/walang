#include "variant_type.hpp"
#include "ast/expression.hpp"
#include "ast/op.hpp"
#include "helper/overload.hpp"
#include "ir/function.hpp"
#include <binaryen-c.h>
#include <cstdint>
#include <fmt/core.h>
#include <magic_enum.hpp>
#include <memory>
#include <stdexcept>

namespace walang {
namespace ir {

VariantType::VariantType(Type typeName) : typeName_(typeName) {}

std::string VariantType::to_string() const { return fmt::format("{}", magic_enum::enum_name(typeName_)); }

std::shared_ptr<VariantType> const &PendingResolveType::resolvedType() const {
  if (resolvedType_ == nullptr) {
    throw std::runtime_error("unresolved type '" + to_string() + "'");
  }
  return resolvedType_;
}

std::shared_ptr<VariantType> VariantType::resolveType(std::string const &typeName) {
  if (typeName == "i32") {
    return std::make_shared<TypeI32>();
  } else if (typeName == "u32") {
    return std::make_shared<TypeU32>();
  } else if (typeName == "i64") {
    return std::make_shared<TypeI64>();
  } else if (typeName == "u64") {
    return std::make_shared<TypeU64>();
  } else if (typeName == "f32") {
    return std::make_shared<TypeF32>();
  } else if (typeName == "f64") {
    return std::make_shared<TypeF64>();
  }
  throw std::runtime_error("unknown type " + typeName);
}
std::shared_ptr<VariantType> VariantType::inferType(std::shared_ptr<ast::Expression> const &initExpr) {
  if (initExpr->type() == ast::Expression::Type::Identifier) {
    auto identifer = std::dynamic_pointer_cast<ast::Identifier>(initExpr);
    return std::visit(overloaded{[](uint64_t i) -> std::shared_ptr<VariantType> { return std::make_shared<TypeI32>(); },
                                 [](double d) -> std::shared_ptr<VariantType> {
                                   throw std::runtime_error("not support" __FILE__ "#" + std::to_string(__LINE__));
                                 },
                                 [](const std::string &s) -> std::shared_ptr<VariantType> {
                                   throw std::runtime_error("not support" __FILE__ "#" + std::to_string(__LINE__));
                                 }},
                      identifer->id());
  }
  throw std::runtime_error("cannot infer type from " + initExpr->to_string());
}

BinaryenType VariantType::underlyingTypeName() const {
  throw std::runtime_error("underlyingTypeName not supported type '" + to_string() + "'");
}
BinaryenType PendingResolveType::underlyingTypeName() const { return resolvedType()->underlyingTypeName(); }
BinaryenType TypeNone::underlyingTypeName() const { return BinaryenTypeNone(); }
BinaryenType Int32::underlyingTypeName() const { return BinaryenTypeInt32(); }
BinaryenType Int64::underlyingTypeName() const { return BinaryenTypeInt64(); }
BinaryenType TypeF32::underlyingTypeName() const { return BinaryenTypeFloat32(); }
BinaryenType TypeF64::underlyingTypeName() const { return BinaryenTypeFloat64(); }

BinaryenExpressionRef VariantType::underlyingDefaultValue(BinaryenModuleRef module) const {
  if (underlyingTypeName() == BinaryenTypeInt32()) {
    return BinaryenConst(module, BinaryenLiteralInt32(0));
  } else if (underlyingTypeName() == BinaryenTypeInt64()) {
    return BinaryenConst(module, BinaryenLiteralInt64(0));
  } else if (underlyingTypeName() == BinaryenTypeFloat32()) {
    return BinaryenConst(module, BinaryenLiteralFloat32(0));
  } else if (underlyingTypeName() == BinaryenTypeFloat64()) {
    return BinaryenConst(module, BinaryenLiteralFloat64(0));
  } else {
    assert(false);
  }
}
BinaryenExpressionRef VariantType::underlyingConst(BinaryenModuleRef module, int64_t value) const {
  if (underlyingTypeName() == BinaryenTypeInt32()) {
    return BinaryenConst(module, BinaryenLiteralInt32(static_cast<int32_t>(value)));
  } else if (underlyingTypeName() == BinaryenTypeInt64()) {
    return BinaryenConst(module, BinaryenLiteralInt64(value));
  } else if (underlyingTypeName() == BinaryenTypeFloat32()) {
    return BinaryenConst(module, BinaryenLiteralFloat32(static_cast<float>(value)));
  } else if (underlyingTypeName() == BinaryenTypeFloat64()) {
    return BinaryenConst(module, BinaryenLiteralFloat64(static_cast<double>(value)));
  } else {
    assert(false);
  }
}
BinaryenExpressionRef VariantType::underlyingConst(BinaryenModuleRef module, double value) const {
  if (underlyingTypeName() == BinaryenTypeInt32()) {
    throw std::runtime_error("invalid convert from double to i32");
  } else if (underlyingTypeName() == BinaryenTypeInt64()) {
    throw std::runtime_error("invalid convert from double to i64");
  } else if (underlyingTypeName() == BinaryenTypeFloat32()) {
    return BinaryenConst(module, BinaryenLiteralFloat32(static_cast<float>(value)));
  } else if (underlyingTypeName() == BinaryenTypeFloat64()) {
    return BinaryenConst(module, BinaryenLiteralFloat64(value));
  } else {
    assert(false);
  }
}

bool VariantType::tryResolveTo(std::shared_ptr<VariantType> const &type) const {
  if (type->typeName_ == typeName_) {
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
  if (type->typeName() == Type::I32 || type->typeName() == Type::U32 || type->typeName() == Type::I64 ||
      type->typeName() == Type::U64) {
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
  throw std::runtime_error(
      fmt::format("invalid prefix operator '{0}' for '{1}'", ast::Operator::to_string(op), to_string()));
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
  assert(false);
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
  assert(false);
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
    throw std::runtime_error(
        fmt::format("invalid prefix operator '{0}' for '{1}'", ast::Operator::to_string(op), to_string()));
  }
  }
  assert(false);
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
    throw std::runtime_error(
        fmt::format("invalid prefix operator '{0}' for '{1}'", ast::Operator::to_string(op), to_string()));
  }
  }
  assert(false);
}

BinaryenExpressionRef PendingResolveType::handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                                         BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                                         std::shared_ptr<Function> const &function) {
  return resolvedType()->handleBinaryOp(module, op, leftRef, rightRef, function);
}
BinaryenExpressionRef TypeNone::handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                               BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                               std::shared_ptr<Function> const &function) {
  throw std::runtime_error(
      fmt::format("invalid prefix operator '{0}' for '{1}'", ast::Operator::to_string(op), to_string()));
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
    auto loadLeftRexprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, rightRef, loadLeftRexprResult);
  }
  case ast::BinaryOp::LOGIC_OR: {
    auto tempLocal = function->addTempLocal(shared_from_this());
    auto conditionalExprRef = BinaryenLocalTee(module, tempLocal->index(), leftRef, underlyingTypeName());
    auto loadLeftRexprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, loadLeftRexprResult, rightRef);
  }
  }
  assert(false);
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
    auto loadLeftRexprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, rightRef, loadLeftRexprResult);
  }
  case ast::BinaryOp::LOGIC_OR: {
    auto tempLocal = function->addTempLocal(shared_from_this());
    auto conditionalExprRef = BinaryenLocalTee(module, tempLocal->index(), leftRef, underlyingTypeName());
    auto loadLeftRexprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, loadLeftRexprResult, rightRef);
  }
  }
  assert(false);
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
    auto loadLeftRexprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, rightRef, loadLeftRexprResult);
  }
  case ast::BinaryOp::LOGIC_OR: {
    auto tempLocal = function->addTempLocal(shared_from_this());
    auto conditionalExprRef = BinaryenLocalTee(module, tempLocal->index(), leftRef, underlyingTypeName());
    auto loadLeftRexprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, loadLeftRexprResult, rightRef);
  }
  }
  assert(false);
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
    auto loadLeftRexprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, rightRef, loadLeftRexprResult);
  }
  case ast::BinaryOp::LOGIC_OR: {
    auto tempLocal = function->addTempLocal(shared_from_this());
    auto conditionalExprRef = BinaryenLocalTee(module, tempLocal->index(), leftRef, underlyingTypeName());
    auto loadLeftRexprResult = BinaryenLocalGet(module, tempLocal->index(), underlyingTypeName());
    return BinaryenIf(module, conditionalExprRef, loadLeftRexprResult, rightRef);
  }
  }
  assert(false);
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
    throw std::runtime_error(
        fmt::format("invalid operator '{0}' for '{1}'", ast::Operator::to_string(op), to_string()));
  }
  }
  assert(false);
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
    throw std::runtime_error(
        fmt::format("invalid operator '{0}' for '{1}'", ast::Operator::to_string(op), to_string()));
  }
  }
  assert(false);
}

} // namespace ir
} // namespace walang