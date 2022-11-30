#pragma once

#include "ast/expression.hpp"
#include "ast/op.hpp"
#include "ast/statement.hpp"
#include <binaryen-c.h>
#include <cstdint>
#include <memory>
#include <string>

namespace walang {
namespace ir {

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
  };
  static std::shared_ptr<VariantType> const &resolveType(std::string const &name);
  static std::shared_ptr<VariantType> const &inferType(std::shared_ptr<ast::Expression> const &initExpr);
  static std::shared_ptr<VariantType> const &getTypeFromDeclare(ast::DeclareStatement const &declare);

  virtual ~VariantType() = default;

  Type type() const noexcept { return type_; }
  virtual std::string to_string() const;
  virtual BinaryenType underlyingTypeName() const = 0;
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

  VariantType(Type typeName);
};

class PendingResolveType : public VariantType {
public:
  std::shared_ptr<VariantType> const &resolvedType() const;
  bool isResolved() const noexcept { return resolvedType_ != nullptr; }
  virtual BinaryenType underlyingTypeName() const override;

  virtual BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                               BinaryenExpressionRef exprRef) const override;
  virtual BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                               BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                               std::shared_ptr<Function> const &function) override;

protected:
  mutable std::shared_ptr<VariantType> resolvedType_{nullptr};

  using VariantType::VariantType;
};

class TypeAuto : public PendingResolveType {
public:
  TypeAuto() : PendingResolveType(Type::Auto) {}
  virtual bool tryResolveTo(std::shared_ptr<VariantType> const &type) const override;
};

class TypeCondition : public PendingResolveType {
public:
  TypeCondition() : PendingResolveType(Type::ConditionType) {}
  virtual bool tryResolveTo(std::shared_ptr<VariantType> const &type) const override;
};

class TypeNone : public VariantType {
public:
  TypeNone() : VariantType(Type::None) {}
  virtual BinaryenType underlyingTypeName() const override;

  virtual BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                               BinaryenExpressionRef exprRef) const override;
  virtual BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                               BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                               std::shared_ptr<Function> const &function) override;
};

class Int32 : public VariantType {
public:
  virtual BinaryenType underlyingTypeName() const;

  virtual BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                               BinaryenExpressionRef exprRef) const;

protected:
  std::shared_ptr<VariantType> resolvedType_{nullptr};

  using VariantType::VariantType;
};

class TypeI32 : public Int32 {
public:
  TypeI32() : Int32(Type::I32) {}
  virtual BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                               BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                               std::shared_ptr<Function> const &function) override;
};
class TypeU32 : public Int32 {
public:
  TypeU32() : Int32(Type::U32) {}
  virtual BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                               BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                               std::shared_ptr<Function> const &function) override;
};

class Int64 : public VariantType {
public:
  virtual BinaryenType underlyingTypeName() const;
  virtual BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                               BinaryenExpressionRef exprRef) const;

protected:
  std::shared_ptr<VariantType> resolvedType_{nullptr};

  using VariantType::VariantType;
};

class TypeI64 : public Int64 {
public:
  TypeI64() : Int64(Type::I64) {}
  virtual BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                               BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                               std::shared_ptr<Function> const &function) override;
};

class TypeU64 : public Int64 {
public:
  TypeU64() : Int64(Type::U64) {}
  virtual BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                               BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                               std::shared_ptr<Function> const &function) override;
};

class TypeF32 : public VariantType {
public:
  TypeF32() : VariantType(Type::F32) {}
  virtual BinaryenType underlyingTypeName() const override;

  virtual BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                               BinaryenExpressionRef exprRef) const override;
  virtual BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                               BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                               std::shared_ptr<Function> const &function) override;
};

class TypeF64 : public VariantType {
public:
  TypeF64() : VariantType(Type::F64) {}
  virtual BinaryenType underlyingTypeName() const override;

  virtual BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                               BinaryenExpressionRef exprRef) const override;
  virtual BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                               BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                               std::shared_ptr<Function> const &function) override;
};

class Signature : public VariantType {
public:
  Signature(std::vector<std::shared_ptr<VariantType>> const &argumentTypes,
            std::shared_ptr<VariantType> const &returnType);
  virtual std::string to_string() const override;
  virtual BinaryenType underlyingTypeName() const override;
  virtual BinaryenExpressionRef handlePrefixOp(BinaryenModuleRef module, ast::PrefixOp op,
                                               BinaryenExpressionRef exprRef) const override;
  virtual BinaryenExpressionRef handleBinaryOp(BinaryenModuleRef module, ast::BinaryOp op,
                                               BinaryenExpressionRef leftRef, BinaryenExpressionRef rightRef,
                                               std::shared_ptr<Function> const &function) override;

  std::vector<std::shared_ptr<VariantType>> const &argumentTypes() const { return argumentTypes_; }
  std::shared_ptr<VariantType> const &returnType() const { return returnType_; }

private:
  std::vector<std::shared_ptr<VariantType>> argumentTypes_;
  std::shared_ptr<VariantType> returnType_;
};

} // namespace ir
} // namespace walang