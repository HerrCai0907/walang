#include "diagnose.hpp"
#include <fmt/core.h>
#include <stdexcept>
#include <utility>

namespace walang {

TypeConvertError::TypeConvertError(std::string from, std::string to)
    : CompilerError(), from_(std::move(from)), to_(std::move(to)) {}
void TypeConvertError::generateErrorMessage() {
  errorMessage_ = fmt::format("invalid convert from {0} to {1}\n\t{2}", from_, to_, range_);
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
RecursiveDefinedSymbol::RecursiveDefinedSymbol(std::string symbol) : CompilerError(), symbol_(std::move(symbol)) {}
void RecursiveDefinedSymbol::generateErrorMessage() {
  errorMessage_ = fmt::format("recursive defined symbol {0} \n\t{1}", symbol_, range_);
}

CannotResolveSymbol::CannotResolveSymbol() : CompilerError() {}
void CannotResolveSymbol::generateErrorMessage() { errorMessage_ = fmt::format("cannot resolve symbol\n\t{}", range_); }

ErrorDecorator::ErrorDecorator(std::string decorator) : CompilerError(), decorator_(std::move(decorator)) {}
void ErrorDecorator::generateErrorMessage() {
  if (decorator_ == "readonly") {
    errorMessage_ = fmt::format("'readonly' decorator can only be used in class method \n\t{}", range_);
  } else {
    errorMessage_ = fmt::format("error decorator '{}' \n\t{}", decorator_, range_);
  }
}

} // namespace walang
