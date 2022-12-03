#pragma once

#include "ast/expression.hpp"
#include "ast/op.hpp"
#include "ast/statement.hpp"
#include <binaryen-c.h>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace walang::ir {

class Function;

class VariantType : public std::enable_shared_from_this<VariantType> {
public:
  enum class Type {
    Auto,
    None,
    I32,
    U32,
    I64,
    U64,
    F32,
    F64,
    ConditionType,
    Signature,
    Class,
  };

  virtual ~VariantType() = default;

  Type type() const noexcept { return type_; }
  virtual std::string to_string() const;

  static std::shared_ptr<VariantType> from(BinaryenType t);

  virtual BinaryenType underlyingType() const = 0;
  [[nodiscard]] virtual std::vector<BinaryenType> underlyingTypes() const { return {underlyingType()}; }
  enum class UnderlyingReturnTypeStatus { None, LoadFromMemory, ByReturnValue };
  [[nodiscard]] UnderlyingReturnTypeStatus underlyingReturnTypeStatus() const;
  BinaryenExpressionRef underlyingDefaultValue(BinaryenModuleRef module) const;
  BinaryenExpressionRef underlyingConst(BinaryenModuleRef module, int64_t value) const;
  BinaryenExpressionRef underlyingConst(BinaryenModuleRef module, double value) const;

  virtual bool tryResolveTo(std::shared_ptr<VariantType> const &type) const;

  virtual BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                               BinaryenExpressionRef exprRef) const = 0;
  virtual BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                               BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                               std::shared_ptr<Function> const &function) = 0;

protected:
  Type type_;

  explicit VariantType(Type type);
};

class PendingResolveType : public VariantType {
public:
  std::shared_ptr<VariantType> const &resolvedType() const;
  bool isResolved() const noexcept { return resolvedType_ != nullptr; }
  BinaryenType underlyingType() const override;

  BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                       BinaryenExpressionRef exprRef) const override;
  BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                       BinaryenExpressionRef rightRef,
                                       std::shared_ptr<Function> const &function) override;

protected:
  mutable std::shared_ptr<VariantType> resolvedType_{nullptr};

  using VariantType::VariantType;
};

class TypeAuto : public PendingResolveType {
public:
  TypeAuto() : PendingResolveType(Type::Auto) {}
  bool tryResolveTo(std::shared_ptr<VariantType> const &type) const override;
};

class TypeCondition : public PendingResolveType {
public:
  TypeCondition() : PendingResolveType(Type::ConditionType) {}
  bool tryResolveTo(std::shared_ptr<VariantType> const &type) const override;
};

class TypeNone : public VariantType {
public:
  TypeNone() : VariantType(Type::None) {}
  BinaryenType underlyingType() const override;

  BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                       BinaryenExpressionRef exprRef) const override;
  BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                       BinaryenExpressionRef rightRef,
                                       std::shared_ptr<Function> const &function) override;
};

class Int32 : public VariantType {
public:
  BinaryenType underlyingType() const override;

  BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                       BinaryenExpressionRef exprRef) const override;

protected:
  std::shared_ptr<VariantType> resolvedType_{nullptr};

  using VariantType::VariantType;
};

class TypeI32 : public Int32 {
public:
  TypeI32() : Int32(Type::I32) {}
  BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                       BinaryenExpressionRef rightRef,
                                       std::shared_ptr<Function> const &function) override;
};
class TypeU32 : public Int32 {
public:
  TypeU32() : Int32(Type::U32) {}
  BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                       BinaryenExpressionRef rightRef,
                                       std::shared_ptr<Function> const &function) override;
};

class Int64 : public VariantType {
public:
  BinaryenType underlyingType() const override;
  BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                       BinaryenExpressionRef exprRef) const override;

protected:
  std::shared_ptr<VariantType> resolvedType_{nullptr};

  using VariantType::VariantType;
};

class TypeI64 : public Int64 {
public:
  TypeI64() : Int64(Type::I64) {}
  BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                       BinaryenExpressionRef rightRef,
                                       std::shared_ptr<Function> const &function) override;
};

class TypeU64 : public Int64 {
public:
  TypeU64() : Int64(Type::U64) {}
  BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                       BinaryenExpressionRef rightRef,
                                       std::shared_ptr<Function> const &function) override;
};

class TypeF32 : public VariantType {
public:
  TypeF32() : VariantType(Type::F32) {}
  BinaryenType underlyingType() const override;

  BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                       BinaryenExpressionRef exprRef) const override;
  BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                       BinaryenExpressionRef rightRef,
                                       std::shared_ptr<Function> const &function) override;
};

class TypeF64 : public VariantType {
public:
  TypeF64() : VariantType(Type::F64) {}
  BinaryenType underlyingType() const override;

  BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                       BinaryenExpressionRef exprRef) const override;
  BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                       BinaryenExpressionRef rightRef,
                                       std::shared_ptr<Function> const &function) override;
};

class Signature : public VariantType {
public:
  Signature(std::vector<std::shared_ptr<VariantType>> argumentTypes, std::shared_ptr<VariantType> returnType);
  std::string to_string() const override;
  BinaryenType underlyingType() const override;
  BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                       BinaryenExpressionRef exprRef) const override;
  BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                       BinaryenExpressionRef rightRef,
                                       std::shared_ptr<Function> const &function) override;

  std::vector<std::shared_ptr<VariantType>> const &argumentTypes() const { return argumentTypes_; }
  std::shared_ptr<VariantType> const &returnType() const { return returnType_; }

private:
  std::vector<std::shared_ptr<VariantType>> argumentTypes_;
  std::shared_ptr<VariantType> returnType_;
};

class Class : public VariantType {
public:
  struct ClassMember {
    std::string memberName_;
    std::shared_ptr<VariantType> memberType_;
  };

  explicit Class(std::string className);
  void setMembers(std::vector<ClassMember> members) { member_ = std::move(members); }
  void setMethodMap(std::map<std::string, std::shared_ptr<Function>> methodMap) { methodMap_ = std::move(methodMap); }

  std::string to_string() const override;
  BinaryenType underlyingType() const override;
  std::vector<BinaryenType> underlyingTypes() const override;

  BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                       BinaryenExpressionRef exprRef) const override;
  BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op, BinaryenExpressionRef leftRef,
                                       BinaryenExpressionRef rightRef,
                                       std::shared_ptr<Function> const &function) override;
  [[nodiscard]] std::string const &className() { return className_; }
  [[nodiscard]] std::vector<ClassMember> const &member() { return member_; }
  [[nodiscard]] std::map<std::string, std::shared_ptr<Function>> const &methodMap() { return methodMap_; }

private:
  std::string className_;
  std::vector<ClassMember> member_{};
  std::map<std::string, std::shared_ptr<Function>> methodMap_{};
};

} // namespace walang::ir