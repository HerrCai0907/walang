#pragma once

#include "ast/statement.hpp"
#include "ir/variant_type.hpp"
#include <binaryen-c.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace walang::ir {

class Symbol {
public:
  enum class Type {
    TypeGlobal,
    TypeLocal,
    TypeFunction,
  };
  explicit Symbol(Type type, std::shared_ptr<VariantType> const &variantType)
      : type_(type), variantType_(variantType) {}
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

} // namespace walang
