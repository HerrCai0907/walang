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
  virtual ~Expression() = default;
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
  virtual ~PrefixExpression() override {}

private:
  Op op_;
  std::shared_ptr<Expression> expr_;
};

class BinaryExpression final : public Expression {
public:
  BinaryExpression() noexcept;
  BinaryExpression(walangParser::BinaryExpressionContext *ctx,
                   std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map);
  virtual ~BinaryExpression() = default;
  virtual std::string to_string() const override;
  Op op() const noexcept { return op_; }
  std::shared_ptr<Expression> const &leftExpr() const noexcept { return leftExpr_; }
  std::shared_ptr<Expression> const &rightExpr() const noexcept { return rightExpr_; }

private:
  Op op_;
  std::shared_ptr<Expression> leftExpr_;
  std::shared_ptr<Expression> rightExpr_;
};

class TernaryExpression : public Expression {
public:
  virtual ~TernaryExpression() override {}

private:
  std::shared_ptr<Expression> conditionExpr_;
  std::shared_ptr<Expression> leftExpr_;
  std::shared_ptr<Expression> rightExpr_;
};

} // namespace ast
} // namespace walang
