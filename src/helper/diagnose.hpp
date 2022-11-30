#pragma once

#include "helper/range.hpp"
#include "ir/variant_type.hpp"
#include <exception>
#include <memory>
#include <stdexcept>

namespace walang {

class CompilerError : public std::exception {
public:
  CompilerError() = default;
  explicit CompilerError(ast::Range const &range) : range_(range) {}

  virtual const char *what() const noexcept = 0;
  void setRange(ast::Range const &range) {
    range_ = range;
    generateErrorMessage();
  }

protected:
  std::string errorMessage_{};
  ast::Range range_{};

  virtual void generateErrorMessage() = 0;
};

class TypeConvertError : public CompilerError {
public:
  TypeConvertError(std::shared_ptr<ir::VariantType const> const &from,
                   std::shared_ptr<ir::VariantType const> const &to);
  TypeConvertError(std::shared_ptr<ir::VariantType const> const &from, std::shared_ptr<ir::VariantType const> const &to,
                   ast::Range const &range);

  virtual const char *what() const noexcept override;

private:
  std::shared_ptr<ir::VariantType const> const from_;
  std::shared_ptr<ir::VariantType const> const to_;

  virtual void generateErrorMessage() override;
};

} // namespace walang