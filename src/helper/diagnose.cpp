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
  errorMessage_ =
      fmt::format("invalid operator '{0}' for '{1}'\n\t{2}", ast::Operator::to_string(op_), type_->to_string(), range_);
}

ArgumentCountError::ArgumentCountError(uint32_t expected, uint32_t actual, ast::Range const &range)
    : CompilerError(range), expected_(expected), actual_(actual) {
  generateErrorMessage();
}
void ArgumentCountError::generateErrorMessage() {
  errorMessage_ = fmt::format("expect '{0}' arguments but get '{1}'\n\t{2}", expected_, actual_, range_);
}

JumpStatementError::JumpStatementError(std::string const &statement) : CompilerError(), statement_(statement) {}
void JumpStatementError::generateErrorMessage() {
  errorMessage_ = fmt::format("invalid {0} statement \n\t{1}", statement_, range_);
}

} // namespace walang