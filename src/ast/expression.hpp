#pragma once

#include "op.hpp"
#include <memory>
#include <string>

namespace walang {
namespace ast {

class Expression {
public:
  virtual ~Expression() = 0;
};

class Identifier : public Expression {
public:
  virtual ~Identifier() {}

private:
  std::string name_;
};

class PrefixExpression : public Expression {
public:
  virtual ~PrefixExpression() override {}

private:
  Op op_;
  std::shared_ptr<Expression> expr_;
};

class BinaryExpression : public Expression {
public:
  virtual ~BinaryExpression() override {}

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
