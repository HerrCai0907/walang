#pragma once

#include "expression.hpp"
#include "generated/walangParser.h"
#include "node.hpp"
#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace walang {
namespace ast {

class Statement : public Node {
public:
  enum Type { _DeclareStatement, _AssignStatement, _ExpressionStatement, _BlockStatement, _IfStatement };
  Statement(Type type) noexcept : type_(type) {}
  virtual ~Statement() = default;

  Type type() const noexcept { return type_; }

private:
  Type type_;
};

class DeclareStatement final : public Statement {
public:
  DeclareStatement(walangParser::DeclareStatementContext *ctx,
                   std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map);
  virtual ~DeclareStatement() = default;
  std::string to_string() const override;
  std::string name() const noexcept { return name_; }
  std::shared_ptr<Expression> init() const noexcept { return initExpr_; }

private:
  std::string name_;
  std::shared_ptr<Expression> initExpr_;
};

class AssignStatement final : public Statement {
public:
  AssignStatement(walangParser::AssignStatementContext *ctx,
                  std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map);
  virtual ~AssignStatement() = default;
  std::string to_string() const override;
  std::shared_ptr<Expression> variant() const noexcept { return varExpr_; }
  std::shared_ptr<Expression> value() const noexcept { return valueExpr_; }

private:
  std::shared_ptr<Expression> varExpr_;
  std::shared_ptr<Expression> valueExpr_;
};

class ExpressionStatement : public Statement {
public:
  ExpressionStatement(walangParser::ExpressionStatementContext *ctx,
                      std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map);
  virtual ~ExpressionStatement() = default;
  std::string to_string() const;
  std::shared_ptr<Expression> expr() const noexcept { return expr_; }

private:
  std::shared_ptr<Expression> expr_;
};

class BlockStatement : public Statement {
public:
  BlockStatement(walangParser::BlockStatementContext *ctx,
                 std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map);
  virtual ~BlockStatement() = default;
  std::string to_string() const;
  std::vector<std::shared_ptr<Statement>> const &statements() const noexcept { return statements_; }

private:
  std::vector<std::shared_ptr<Statement>> statements_;
};

class IfStatement : public Statement {
public:
  IfStatement(walangParser::IfStatementContext *ctx,
              std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map);
  virtual ~IfStatement() = default;
  std::string to_string() const;
  std::shared_ptr<Expression> const &condition() const noexcept { return condition_; }
  std::shared_ptr<BlockStatement> const &thenBlock() const noexcept { return thenBlock_; }
  std::shared_ptr<BlockStatement> const &elseBlock() const noexcept { return elseBlock_; }
  std::shared_ptr<IfStatement> const &elseIfStatement() const noexcept { return elseif_; }

private:
  std::shared_ptr<Expression> condition_;
  std::shared_ptr<BlockStatement> thenBlock_;
  std::shared_ptr<BlockStatement> elseBlock_;
  std::shared_ptr<IfStatement> elseif_;
};

} // namespace ast
} // namespace walang