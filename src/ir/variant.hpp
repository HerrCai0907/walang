#pragma once

#include "ast/statement.hpp"
#include "ir/variant_type.hpp"
#include <binaryen-c.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace walang {
namespace ir {

class Variant {
public:
  enum class Type {
    _Global,
    _Local,
  };
  explicit Variant(Type type, std::shared_ptr<VariantType> const &variantType)
      : type_(type), variantType_(variantType) {}
  virtual ~Variant() = default;

  Type type() const noexcept { return type_; }
  std::shared_ptr<VariantType> const &variantType() const noexcept { return variantType_; }
  virtual BinaryenExpressionRef makeAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) = 0;

private:
  Type type_;
  std::shared_ptr<VariantType> variantType_;
};

class Global : public Variant {
public:
  explicit Global(ast::DeclareStatement const &declare);
  virtual ~Global() override = default;
  std::string name() const noexcept { return name_; }
  virtual BinaryenExpressionRef makeAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) override;

private:
  std::string name_;
  std::shared_ptr<ast::Expression> init_;
};

class Local : public Variant {
public:
  explicit Local(uint32_t index, std::shared_ptr<VariantType> const &type)
      : Variant(Type::_Local, type), index_{index}, name_{}, init_{nullptr} {}
  ~Local() = default;

  uint32_t index() const noexcept { return index_; }
  std::string name() const noexcept { return name_; }
  virtual BinaryenExpressionRef makeAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) override;

private:
  uint32_t index_;
  std::string name_;
  std::shared_ptr<ast::Expression> init_;
};

} // namespace ir
} // namespace walang
