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

class CompilerErrorBase : public std::exception {
public:
  [[nodiscard]] const char *what() const noexcept override { return errorMessage_.c_str(); }
  void setRange(ast::Range const &range) {
    if (errorMessage_.empty()) {
      range_ = range;
      generateErrorMessage();
    }
  }
  [[noreturn]] virtual void setRangeAndThrow(ast::Range const &range) = 0;

protected:
  std::string errorMessage_{};
  ast::Range range_{};

  virtual void generateErrorMessage() = 0;
};

template <class Child> class CompilerError : public CompilerErrorBase {
public:
  CompilerError() = default;

  [[noreturn]] void setRangeAndThrow(ast::Range const &range) override {
    setRange(range);
    throw *dynamic_cast<Child *>(this);
  }
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

class CannotResolveSymbol : public CompilerError<CannotResolveSymbol> {
public:
  explicit CannotResolveSymbol();

private:
  void generateErrorMessage() override;
};

} // namespace walang