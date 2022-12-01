#include "diagnose.hpp"
#include "fmt/core.h"
#include <stdexcept>

namespace walang {

TypeConvertError::TypeConvertError(std::shared_ptr<ir::VariantType const> const &from,
                                   std::shared_ptr<ir::VariantType const> const &to)
    : CompilerError(), from_(from), to_(to) {}
TypeConvertError::TypeConvertError(std::shared_ptr<ir::VariantType const> const &from,
                                   std::shared_ptr<ir::VariantType const> const &to, ast::Range const &range)
    : CompilerError(range), from_(from), to_(to) {
  generateErrorMessage();
}
void TypeConvertError::generateErrorMessage() {
  errorMessage_ = fmt::format("invalid convert from {0} to {1}\n\t{2}", from_->to_string(), to_->to_string(), range_);
}

InvalidOperator::InvalidOperator(std::shared_ptr<ir::VariantType const> const &type, ast::PrefixOp op)
    : type_(type), op_(op) {}
InvalidOperator::InvalidOperator(std::shared_ptr<ir::VariantType const> const &type, ast::BinaryOp op)
    : type_(type), op_(op) {}
void InvalidOperator::generateErrorMessage() {
  errorMessage_ = fmt::format("invalid operator '{0}' for '{1}'", ast::Operator::to_string(op_), type_->to_string());
}

} // namespace walang