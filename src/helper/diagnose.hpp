#pragma once

#include "ast/op.hpp"
#include "helper/range.hpp"
#include "ir/variant_type.hpp"
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace walang {

class CompilerError : public std::exception {
public:
  CompilerError() = default;

  [[nodiscard]] const char *what() const noexcept override { return errorMessage_.c_str(); }
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
  TypeConvertError(std::string from, std::string to);

private:
  std::string from_;
  std::string to_;

  void generateErrorMessage() override;
};

class InvalidOperator : public CompilerError {
public:
  InvalidOperator(std::shared_ptr<ir::VariantType const> type, ast::PrefixOp op);
  InvalidOperator(std::shared_ptr<ir::VariantType const> type, ast::BinaryOp op);

private:
  std::variant<ast::PrefixOp, ast::BinaryOp> const op_;
  std::shared_ptr<ir::VariantType const> const type_;

  void generateErrorMessage() override;
};

class ArgumentCountError : public CompilerError {
public:
  ArgumentCountError(uint32_t expected, uint32_t actual);

private:
  uint32_t expected_;
  uint32_t actual_;

  void generateErrorMessage() override;
};

class JumpStatementError : public CompilerError {
public:
  explicit JumpStatementError(std::string statement);

private:
  std::string statement_;

  void generateErrorMessage() override;
};

class RedefinedSymbol : public CompilerError {
public:
  explicit RedefinedSymbol(std::string symbol);

private:
  std::string symbol_;

  void generateErrorMessage() override;
};

class UnknownSymbol : public CompilerError {
public:
  explicit UnknownSymbol(std::string symbol);

private:
  std::string symbol_;

  void generateErrorMessage() override;
};

class RecursiveDefinedSymbol : public CompilerError {
public:
  explicit RecursiveDefinedSymbol(std::string symbol);

private:
  std::string symbol_;

  void generateErrorMessage() override;
};

} // namespace walang