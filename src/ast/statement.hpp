#pragma once

#include "expression.hpp"
#include "generated/walangParser.h"
#include "node.hpp"
#include <cassert>
#include <memory>
#include <string>

namespace walang {
namespace ast {

class Statement : public Node {
public:
  virtual ~Statement() = default;

private:
};

class DeclareStatement final : public Statement {
public:
  virtual ~DeclareStatement() = default;
  void update(walangParser::DeclareStatementContext *ctx,
              std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map);
  std::string to_string() const override;
  std::string name() const noexcept { return name_; }
  std::shared_ptr<Expression> init() const noexcept { return initExpr_; }

private:
  std::string name_;
  std::shared_ptr<Expression> initExpr_;
};

class AssignStatement final : public Statement {
public:
  virtual ~AssignStatement() = default;
  void update(walangParser::AssignStatementContext *ctx,
              std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map);
  std::string to_string() const override;
  std::shared_ptr<Expression> variant() const noexcept { return varExpr_; }
  std::shared_ptr<Expression> value() const noexcept { return valueExpr_; }

private:
  std::shared_ptr<Expression> varExpr_;
  std::shared_ptr<Expression> valueExpr_;
};

class ExpressionStatement : public Statement {
public:
  virtual ~ExpressionStatement() = default;

private:
  std::shared_ptr<Expression> expr_;
};

} // namespace ast
} // namespace walang