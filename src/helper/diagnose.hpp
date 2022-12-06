#pragma once

#include "ast/op.hpp"
#include "helper/range.hpp"
#include "ir/variant_type.hpp"
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace walang {

template <class Child> class CompilerError : public std::exception {
public:
  CompilerError() = default;

  [[nodiscard]] const char *what() const noexcept override { return errorMessage_.c_str(); }
  void setRange(ast::Range const &range) {
    range_ = range;
    generateErrorMessage();
  }
  [[noreturn]] void setRangeAndThrow(ast::Range const &range) {
    setRange(range);
    throw *dynamic_cast<Child *>(this);
  }

protected:
  std::string errorMessage_{};
  ast::Range range_{};

  virtual void generateErrorMessage() = 0;
};

class TypeConvertError : public CompilerError<TypeConvertError> {
public:
  TypeConvertError(std::string from, std::string to);

private:
  std::string from_;
  std::string to_;

  void generateErrorMessage() override;
};

class InvalidOperator : public CompilerError<InvalidOperator> {
public:
  InvalidOperator(std::shared_ptr<ir::VariantType const> type, ast::PrefixOp op);
  InvalidOperator(std::shared_ptr<ir::VariantType const> type, ast::BinaryOp op);

private:
  std::variant<ast::PrefixOp, ast::BinaryOp> const op_;
  std::shared_ptr<ir::VariantType const> const type_;

  void generateErrorMessage() override;
};

class ArgumentCountError : public CompilerError<ArgumentCountError> {
public:
  ArgumentCountError(uint32_t expected, uint32_t actual);

private:
  uint32_t expected_;
  uint32_t actual_;

  void generateErrorMessage() override;
};

class JumpStatementError : public CompilerError<JumpStatementError> {
public:
  explicit JumpStatementError(std::string statement);

private:
  std::string statement_;

  void generateErrorMessage() override;
};

class RedefinedSymbol : public CompilerError<RedefinedSymbol> {
public:
  explicit RedefinedSymbol(std::string symbol);

private:
  std::string symbol_;

  void generateErrorMessage() override;
};

class UnknownSymbol : public CompilerError<UnknownSymbol> {
public:
  explicit UnknownSymbol(std::string symbol);

private:
  std::string symbol_;

  void generateErrorMessage() override;
};

class RecursiveDefinedSymbol : public CompilerError<RecursiveDefinedSymbol> {
public:
  explicit RecursiveDefinedSymbol(std::string symbol);

private:
  std::string symbol_;

  void generateErrorMessage() override;
};

class CannotInferType : public CompilerError<CannotInferType> {
public:
  explicit CannotInferType();

private:
  void generateErrorMessage() override;
};

} // namespace walang