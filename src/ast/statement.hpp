#pragma once

#include "expression.hpp"
#include "generated/walangParser.h"
#include "node.hpp"
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace walang::ast {

enum StatementType {
  TypeDeclareStatement,
  TypeAssignStatement,
  TypeExpressionStatement,
  TypeBlockStatement,
  TypeIfStatement,
  TypeWhileStatement,
  TypeBreakStatement,
  TypeContinueStatement,
  TypeReturnStatement,
  TypeFunctionStatement,
  TypeClassStatement,
};

class Statement : public Node {
public:
  explicit Statement(StatementType type) noexcept : type_(type) {}
  ~Statement() override = default;

  [[nodiscard]] StatementType type() const noexcept { return type_; }

private:
  StatementType type_;
};

class DeclareStatement final : public Statement {
public:
  DeclareStatement(walangParser::DeclareStatementContext *ctx,
                   std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  ~DeclareStatement() override = default;
  [[nodiscard]] std::string to_string() const override;
  [[nodiscard]] std::string variantName() const noexcept { return variantName_; }
  [[nodiscard]] std::string variantType() const noexcept { return variantType_; }
  [[nodiscard]] std::shared_ptr<Expression> init() const noexcept { return initExpr_; }

private:
  std::string variantName_;
  std::string variantType_;
  std::shared_ptr<Expression> initExpr_;
};

class AssignStatement final : public Statement {
public:
  AssignStatement(walangParser::AssignStatementContext *ctx,
                  std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  ~AssignStatement() override = default;
  [[nodiscard]] std::string to_string() const override;
  [[nodiscard]] std::shared_ptr<Expression> variant() const noexcept { return varExpr_; }
  [[nodiscard]] std::shared_ptr<Expression> value() const noexcept { return valueExpr_; }

private:
  std::shared_ptr<Expression> varExpr_;
  std::shared_ptr<Expression> valueExpr_;
};

class ExpressionStatement : public Statement {
public:
  ExpressionStatement(walangParser::ExpressionStatementContext *ctx,
                      std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  ~ExpressionStatement() override = default;
  [[nodiscard]] std::string to_string() const override;
  [[nodiscard]] std::shared_ptr<Expression> expr() const noexcept { return expr_; }

private:
  std::shared_ptr<Expression> expr_;
};

class BlockStatement : public Statement {
public:
  BlockStatement(walangParser::BlockStatementContext *ctx,
                 std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  ~BlockStatement() override = default;
  [[nodiscard]] std::string to_string() const override;
  [[nodiscard]] std::vector<std::shared_ptr<Statement>> const &statements() const noexcept { return statements_; }

private:
  std::vector<std::shared_ptr<Statement>> statements_;
};

class IfStatement : public Statement {
public:
  IfStatement(walangParser::IfStatementContext *ctx,
              std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  ~IfStatement() override = default;
  [[nodiscard]] std::string to_string() const override;
  [[nodiscard]] std::shared_ptr<Expression> const &condition() const noexcept { return condition_; }
  [[nodiscard]] std::shared_ptr<BlockStatement> const &thenBlock() const noexcept { return thenBlock_; }
  [[nodiscard]] std::shared_ptr<Statement> const &elseBlock() const noexcept { return elseBlock_; }

private:
  std::shared_ptr<Expression> condition_;
  std::shared_ptr<BlockStatement> thenBlock_;
  std::shared_ptr<Statement> elseBlock_;
};

class WhileStatement : public Statement {
public:
  WhileStatement(walangParser::WhileStatementContext *ctx,
                 std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  ~WhileStatement() override = default;
  [[nodiscard]] std::string to_string() const override;
  [[nodiscard]] std::shared_ptr<Expression> const &condition() const noexcept { return condition_; }
  [[nodiscard]] std::shared_ptr<BlockStatement> const &block() const noexcept { return block_; }

private:
  std::shared_ptr<Expression> condition_;
  std::shared_ptr<BlockStatement> block_;
};

class BreakStatement : public Statement {
public:
  BreakStatement() : Statement(StatementType::TypeBreakStatement) {}
  ~BreakStatement() override = default;
  [[nodiscard]] std::string to_string() const override { return "break\n"; }
};

class ContinueStatement : public Statement {
public:
  ContinueStatement() : Statement(StatementType::TypeContinueStatement) {}
  ~ContinueStatement() override = default;
  [[nodiscard]] std::string to_string() const override { return "continue\n"; }
};

class ReturnStatement : public Statement {
public:
  ReturnStatement(walangParser::ReturnStatementContext *ctx,
                  std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  ~ReturnStatement() override = default;
  [[nodiscard]] std::string to_string() const override;
  [[nodiscard]] std::shared_ptr<Expression> const &expr() { return expr_; }

private:
  std::shared_ptr<Expression> expr_;
};

class FunctionStatement : public Statement {
public:
  struct Argument {
    std::string name_;
    std::string type_;
  };

  FunctionStatement(walangParser::FunctionStatementContext *ctx,
                    std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  ~FunctionStatement() override = default;
  [[nodiscard]] std::string to_string() const override;

  [[nodiscard]] std::string const &name() const noexcept { return name_; }
  [[nodiscard]] std::vector<Argument> const &arguments() const noexcept { return arguments_; }
  [[nodiscard]] std::optional<std::string> const &returnType() const noexcept { return returnType_; }
  [[nodiscard]] std::shared_ptr<BlockStatement> const &body() const noexcept { return body_; };
  [[nodiscard]] std::vector<std::string> const &decorators() const noexcept { return decorators_; };

private:
  std::string name_;
  std::vector<Argument> arguments_;
  std::vector<std::string> decorators_;
  std::optional<std::string> returnType_;
  std::shared_ptr<BlockStatement> body_;
};

class ClassStatement : public Statement {
public:
  struct Member {
    std::string name_;
    std::string type_;
  };

  ClassStatement(walangParser::ClassStatementContext *ctx,
                 std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  ~ClassStatement() override = default;
  [[nodiscard]] std::string to_string() const override;
  [[nodiscard]] std::string const &name() const { return name_; }
  [[nodiscard]] std::vector<Member> const &members() const { return members_; }
  [[nodiscard]] std::vector<std::shared_ptr<FunctionStatement>> const &methods() const { return methods_; }

private:
  std::string name_;
  std::vector<Member> members_;
  std::vector<std::shared_ptr<FunctionStatement>> methods_;
};

} // namespace walang::ast