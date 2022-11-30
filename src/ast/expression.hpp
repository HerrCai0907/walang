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

namespace walang {
namespace ast {

class Expression : public Node {
public:
  enum Type {
    _Identifier,
    _PrefixExpression,
    _BinaryExpression,
    _TernaryExpression,
  };
  Expression(Type type) : type_(type) {}
  virtual ~Expression() = default;

  Type type() const noexcept { return type_; }

private:
  Type type_;
};

class Identifier final : public Expression {
public:
  Identifier(walangParser::IdentifierContext *ctx,
             std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &);
  virtual ~Identifier() = default;
  virtual std::string to_string() const override;

  std::variant<uint64_t, double, std::string> const &id() const noexcept { return id_; }

private:
  std::variant<uint64_t, double, std::string> id_;
};

class PrefixExpression : public Expression {
public:
  PrefixExpression(walangParser::PrefixExpressionContext *ctx,
                   std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  virtual ~PrefixExpression() override {}
  virtual std::string to_string() const override;
  PrefixOp op() const noexcept { return op_; }
  std::shared_ptr<Expression> const &expr() const noexcept { return expr_; }

private:
  PrefixOp op_;
  std::shared_ptr<Expression> expr_;
};

class BinaryExpression final : public Expression {
public:
  BinaryExpression() noexcept;
  BinaryExpression(walangParser::BinaryExpressionContext *ctx,
                   std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  virtual ~BinaryExpression() = default;
  virtual std::string to_string() const override;
  BinaryOp op() const noexcept { return op_; }
  std::shared_ptr<Expression> const &leftExpr() const noexcept { return leftExpr_; }
  std::shared_ptr<Expression> const &rightExpr() const noexcept { return rightExpr_; }

private:
  BinaryOp op_;
  std::shared_ptr<Expression> leftExpr_;
  std::shared_ptr<Expression> rightExpr_;

  void appendExpr(BinaryOp op, std::shared_ptr<Expression> rightExpr);
};

class TernaryExpression : public Expression {
public:
  TernaryExpression() noexcept;
  TernaryExpression(walangParser::TernaryExpressionContext *ctx,
                    std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  virtual ~TernaryExpression() = default;
  virtual std::string to_string() const override;

  std::shared_ptr<Expression> const &conditionExpr() { return conditionExpr_; }
  std::shared_ptr<Expression> const &leftExpr() { return leftExpr_; }
  std::shared_ptr<Expression> const &rightExpr() { return rightExpr_; }

private:
  std::shared_ptr<Expression> conditionExpr_;
  std::shared_ptr<Expression> leftExpr_;
  std::shared_ptr<Expression> rightExpr_;
};

} // namespace ast
} // namespace walang
