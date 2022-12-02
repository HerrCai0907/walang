#include "diagnose.hpp"
#include <fmt/core.h>
#include <stdexcept>
#include <utility>

namespace walang {

TypeConvertError::TypeConvertError(std::shared_ptr<ir::VariantType const> from,
                                   std::shared_ptr<ir::VariantType const> to)
    : CompilerError(), from_(std::move(from)), to_(std::move(to)) {}
void TypeConvertError::generateErrorMessage() {
  errorMessage_ = fmt::format("invalid convert from {0} to {1}\n\t{2}", from_->to_string(), to_->to_string(), range_);
}

InvalidOperator::InvalidOperator(std::shared_ptr<ir::VariantType const> type, ast::PrefixOp op)
    : type_(std::move(type)), op_(op) {}
InvalidOperator::InvalidOperator(std::shared_ptr<ir::VariantType const> type, ast::BinaryOp op)
    : type_(std::move(type)), op_(op) {}
void InvalidOperator::generateErrorMessage() {
  errorMessage_ =
      fmt::format("invalid operator '{0}' for '{1}'\n\t{2}", ast::Operator::to_string(op_), type_->to_string(), range_);
}

ArgumentCountError::ArgumentCountError(uint32_t expected, uint32_t actual)
    : CompilerError(), expected_(expected), actual_(actual) {}
void ArgumentCountError::generateErrorMessage() {
  errorMessage_ = fmt::format("expect '{0}' arguments but get '{1}'\n\t{2}", expected_, actual_, range_);
}

JumpStatementError::JumpStatementError(std::string statement) : CompilerError(), statement_(std::move(statement)) {}
void JumpStatementError::generateErrorMessage() {
  errorMessage_ = fmt::format("invalid {0} statement \n\t{1}", statement_, range_);
}

RedefinedSymbol::RedefinedSymbol(std::string symbol) : CompilerError(), symbol_(std::move(symbol)) {}
void RedefinedSymbol::generateErrorMessage() { errorMessage_ = fmt::format("redefined {0} \n\t{1}", symbol_, range_); }
UnknownSymbol::UnknownSymbol(std::string symbol) : CompilerError(), symbol_(std::move(symbol)) {}
void UnknownSymbol::generateErrorMessage() { errorMessage_ = fmt::format("unknown {0} \n\t{1}", symbol_, range_); }

} // namespace walang