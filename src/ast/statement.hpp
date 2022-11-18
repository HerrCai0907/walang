#pragma once

#include "expression.hpp"
#include <memory>
#include <string>

namespace walang {
namespace ast {

class Statement {
public:
  virtual ~Statement() = 0;

private:
};

class DeclareStatement {
public:
  virtual ~DeclareStatement() {}

private:
  std::string name_;
  std::shared_ptr<Expression> initExpr_;
};

class AssignStatement {
public:
  virtual ~AssignStatement() {}

private:
  std::shared_ptr<Expression> varExpr_;
  std::shared_ptr<Expression> valueExpr_;
};

class ExpressionStatement {
public:
  virtual ~ExpressionStatement() {}

private:
  std::shared_ptr<Expression> expr_;
};

} // namespace ast
} // namespace walang