#pragma once

#include "ast/statement.hpp"
#include "ir/variant_type.hpp"
#include <binaryen-c.h>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace walang::ir {

class Symbol {
public:
  enum class Type {
    TypeGlobal,
    TypeLocal,
    TypeFunction,
  };
  explicit Symbol(Type type, std::shared_ptr<VariantType> variantType)
      : type_(type), variantType_(std::move(variantType)) {}
  virtual ~Symbol() = default;

  [[nodiscard]] Type type() const noexcept { return type_; }
  [[nodiscard]] std::shared_ptr<VariantType> const &variantType() const noexcept { return variantType_; }

protected:
  const Type type_;
  std::shared_ptr<VariantType> variantType_;
};

class Variant : public Symbol {
public:
  explicit Variant(Type type, std::shared_ptr<VariantType> const &variantType) : Symbol(type, variantType) {}
  ~Variant() override = default;

  virtual BinaryenExpressionRef makeAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) = 0;
  virtual BinaryenExpressionRef makeGet(BinaryenModuleRef module) = 0;
};

class Global : public Variant {
public:
  Global(std::string name, std::shared_ptr<VariantType> const &type);
  ~Global() override = default;
  [[nodiscard]] std::string name() const noexcept { return name_; }
  BinaryenExpressionRef makeAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) override;
  BinaryenExpressionRef makeGet(BinaryenModuleRef module) override;

private:
  std::string name_;
};

class Local : public Variant {
public:
  Local(uint32_t index, std::shared_ptr<VariantType> const &type);
  Local(uint32_t index, std::string name, std::shared_ptr<VariantType> const &type);
  ~Local() override = default;

  [[nodiscard]] uint32_t index() const noexcept { return index_; }
  [[nodiscard]] std::string name() const noexcept { return name_; }
  BinaryenExpressionRef makeAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) override;
  BinaryenExpressionRef makeGet(BinaryenModuleRef module) override;

private:
  uint32_t index_;
  std::string name_;
};

class Function : public Symbol {
public:
  Function(std::string name, std::vector<std::string> const &argumentNames,
           std::vector<std::shared_ptr<VariantType>> const &argumentTypes,
           std::shared_ptr<VariantType> const &returnType);

  [[nodiscard]] std::string name() const noexcept { return name_; }
  [[nodiscard]] std::shared_ptr<Signature> signature() const noexcept {
    return std::dynamic_pointer_cast<Signature>(variantType_);
  }

  std::shared_ptr<Local> const &addLocal(std::string const &name, std::shared_ptr<VariantType> const &localType);
  std::shared_ptr<Local> const &addTempLocal(std::shared_ptr<VariantType> const &localType);
  [[nodiscard]] std::shared_ptr<Local> findLocalByName(std::string const &name) const;

  std::string const &createBreakLabel(std::string const &prefix);
  [[nodiscard]] std::string const &topBreakLabel() const;
  void freeBreakLabel();
  std::string const &createContinueLabel(std::string const &prefix);
  [[nodiscard]] std::string const &topContinueLabel() const;
  void freeContinueLabel();

  BinaryenFunctionRef finalize(BinaryenModuleRef module, BinaryenExpressionRef body);

private:
  std::string name_;
  uint32_t argumentSize_;

  std::vector<std::shared_ptr<Local>> locals_{};

  std::stack<std::string> currentBreakLabel_{};
  uint32_t breakLabelIndex_{0U};
  std::stack<std::string> currentContinueLabel_{};
  uint32_t continueLabelIndex_{0U};
};

} // namespace walang::ir
