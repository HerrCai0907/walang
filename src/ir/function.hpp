#pragma once

#include "ast/statement.hpp"
#include "ir/variant.hpp"
#include "ir/variant_type.hpp"
#include <binaryen-c.h>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace walang {
namespace ir {

class Function : public Symbol {
public:
  Function(std::string const &name, std::vector<std::string> const &argumentNames,
           std::vector<std::shared_ptr<VariantType>> const &argumentTypes,
           std::shared_ptr<VariantType> const &returnType);

  std::string name() const noexcept { return name_; }
  std::shared_ptr<Signature> signature() const noexcept { return std::dynamic_pointer_cast<Signature>(variantType_); }

  std::shared_ptr<Local> const &addLocal(std::string const &name, std::shared_ptr<VariantType> const &localType);
  std::shared_ptr<Local> const &addTempLocal(std::shared_ptr<VariantType> const &localType);
  std::shared_ptr<Local> findLocalByName(std::string const &name) const;

  std::string const &createBreakLabel(std::string const &prefix);
  std::string const &topBreakLabel() const;
  void freeBreakLabel();
  std::string const &createContinueLabel(std::string const &prefix);
  std::string const &topContinueLabel() const;
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

} // namespace ir
} // namespace walang
