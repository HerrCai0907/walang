#pragma once

#include "generated/walangParser.h"
#include "node.hpp"
#include "op.hpp"
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace walang::ast {

enum ExpressionType {
  TypeIdentifier,
  TypePrefixExpression,
  TypeBinaryExpression,
  TypeTernaryExpression,
  TypeCallExpression,
};

class Expression : public Node {
public:
  explicit Expression(ExpressionType type) noexcept : type_(type) {}
  ~Expression() override = default;

  [[nodiscard]] ExpressionType type() const noexcept { return type_; }

private:
  ExpressionType type_;
};

class Identifier final : public Expression {
public:
  Identifier(walangParser::IdentifierContext *ctx,
             std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &);
  ~Identifier() override = default;
  [[nodiscard]] std::string to_string() const override;

  [[nodiscard]] std::variant<uint64_t, double, std::string> const &id() const noexcept { return id_; }

private:
  std::variant<uint64_t, double, std::string> id_;
};

class PrefixExpression : public Expression {
public:
  PrefixExpression(walangParser::PrefixExpressionContext *ctx,
                   std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  ~PrefixExpression() override = default;
  [[nodiscard]] std::string to_string() const override;
  [[nodiscard]] PrefixOp op() const noexcept { return op_; }
  [[nodiscard]] std::shared_ptr<Expression> const &expr() const noexcept { return expr_; }

private:
  PrefixOp op_;
  std::shared_ptr<Expression> expr_;
};

class BinaryExpression final : public Expression {
public:
  BinaryExpression() noexcept;
  BinaryExpression(walangParser::BinaryExpressionContext *ctx,
                   std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  ~BinaryExpression() override = default;
  [[nodiscard]] std::string to_string() const override;
  [[nodiscard]] BinaryOp op() const noexcept { return op_; }
  [[nodiscard]] std::shared_ptr<Expression> const &leftExpr() const noexcept { return leftExpr_; }
  [[nodiscard]] std::shared_ptr<Expression> const &rightExpr() const noexcept { return rightExpr_; }

private:
  BinaryOp op_;
  std::shared_ptr<Expression> leftExpr_;
  std::shared_ptr<Expression> rightExpr_;

  void appendExpr(BinaryOp op, const std::shared_ptr<Expression> &rightExpr);
};

class TernaryExpression : public Expression {
public:
  TernaryExpression() noexcept;
  TernaryExpression(walangParser::TernaryExpressionContext *ctx,
                    std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  ~TernaryExpression() override = default;
  [[nodiscard]] std::string to_string() const override;

  std::shared_ptr<Expression> const &conditionExpr() { return conditionExpr_; }
  std::shared_ptr<Expression> const &leftExpr() { return leftExpr_; }
  std::shared_ptr<Expression> const &rightExpr() { return rightExpr_; }

private:
  std::shared_ptr<Expression> conditionExpr_;
  std::shared_ptr<Expression> leftExpr_;
  std::shared_ptr<Expression> rightExpr_;
};

class CallExpression : public Expression {
public:
  CallExpression() noexcept;
  CallExpression(walangParser::CallExpressionContext *ctx,
                 std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  ~CallExpression() override = default;
  [[nodiscard]] std::string to_string() const override;

  [[nodiscard]] std::shared_ptr<Expression> const &caller() const noexcept { return caller_; }
  [[nodiscard]] std::vector<std::shared_ptr<Expression>> const &arguments() const noexcept { return arguments_; }

private:
  std::shared_ptr<Expression> caller_;
  std::vector<std::shared_ptr<Expression>> arguments_;
};

} // namespace walang::ast
