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
  virtual BinaryenExpressionRef makeGet(BinaryenModuleRef module) = 0;

protected:
  const Type type_;
  std::shared_ptr<VariantType> variantType_;
};

class Global : public Variant {
public:
  Global(std::string const &name, std::shared_ptr<VariantType> const &type);
  virtual ~Global() override = default;
  std::string name() const noexcept { return name_; }
  virtual BinaryenExpressionRef makeAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) override;
  virtual BinaryenExpressionRef makeGet(BinaryenModuleRef module) override;

private:
  std::string name_;
};

class Local : public Variant {
public:
  Local(uint32_t index, std::shared_ptr<VariantType> const &type);
  Local(uint32_t index, std::string const &name, std::shared_ptr<VariantType> const &type);
  ~Local() = default;

  uint32_t index() const noexcept { return index_; }
  std::string name() const noexcept { return name_; }
  virtual BinaryenExpressionRef makeAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) override;
  virtual BinaryenExpressionRef makeGet(BinaryenModuleRef module) override;

private:
  uint32_t index_;
  std::string name_;
};

} // namespace ir
} // namespace walang
