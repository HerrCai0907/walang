#include "parser.hpp"
#include "ast/expression.hpp"
#include "ast/file.hpp"
#include "ast/statement.hpp"
#include "generated/walangBaseListener.h"
#include "generated/walangLexer.h"
#include "generated/walangParser.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <utility>

namespace walang {

class Listener : public walangBaseListener {
public:
  explicit Listener(std::shared_ptr<ast::File> file) : file_(std::move(file)) {}

  void exitWalang(walangParser::WalangContext *ctx) override { file_->update(ctx, astNodes_); }

  void exitStatement(walangParser::StatementContext *ctx) override {
    auto *child = dynamic_cast<antlr4::ParserRuleContext *>(ctx->children.at(0));
    assert(astNodes_.count(child) == 1);
    astNodes_.emplace(ctx, astNodes_.find(child)->second);
    astNodes_.find(child)->second->setRange(file_, ctx);
  }
  void exitDeclareStatement(walangParser::DeclareStatementContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::DeclareStatement>(ctx, astNodes_));
  }
  void exitAssignStatement(walangParser::AssignStatementContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::AssignStatement>(ctx, astNodes_));
  }
  void exitExpressionStatement(walangParser::ExpressionStatementContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::ExpressionStatement>(ctx, astNodes_));
  }
  void exitBlockStatement(walangParser::BlockStatementContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::BlockStatement>(ctx, astNodes_));
  }
  void exitIfStatement(walangParser::IfStatementContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::IfStatement>(ctx, astNodes_));
  }
  void exitWhileStatement(walangParser::WhileStatementContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::WhileStatement>(ctx, astNodes_));
  }
  void exitBreakStatement(walangParser::BreakStatementContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::BreakStatement>());
  }
  void exitContinueStatement(walangParser::ContinueStatementContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::ContinueStatement>());
  }
  void exitFunctionStatement(walangParser::FunctionStatementContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::FunctionStatement>(ctx, astNodes_));
  }
  void exitClassStatement(walangParser::ClassStatementContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::ClassStatement>(ctx, astNodes_));
  }

  void exitExpression(walangParser::ExpressionContext *ctx) override {
    auto *child = dynamic_cast<antlr4::ParserRuleContext *>(ctx->children.at(0));
    assert(astNodes_.count(child) == 1);
    astNodes_.emplace(ctx, astNodes_.find(child)->second);
    astNodes_.find(child)->second->setRange(file_, ctx);
  }
  void exitIdentifier(walangParser::IdentifierContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::Identifier>(ctx, astNodes_));
  }
  void exitBinaryExpression(walangParser::BinaryExpressionContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::BinaryExpression>(ctx, astNodes_));
  }
  void exitTernaryExpression(walangParser::TernaryExpressionContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::TernaryExpression>(ctx, astNodes_));
  }
  void exitPrefixExpression(walangParser::PrefixExpressionContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::PrefixExpression>(ctx, astNodes_));
  }
  void exitParenthesesExpression(walangParser::ParenthesesExpressionContext *ctx) override {
    assert(astNodes_.count(ctx->expression()) == 1);
    astNodes_.emplace(ctx, astNodes_.find(ctx->expression())->second);
  }
  void exitCallExpression(walangParser::CallExpressionContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::CallExpression>(ctx, astNodes_));
  }
  void exitMemberExpression(walangParser::MemberExpressionContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::MemberExpression>(ctx, astNodes_));
  }

  void visitErrorNode(antlr4::tree::ErrorNode *node) override {
    std::cerr << "unexpected " << node->getText() << std::endl;
    std::terminate();
  }

private:
  std::shared_ptr<ast::File> file_;
  std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> astNodes_;
};

std::shared_ptr<ast::File> FileParser::parse() {
  auto file = std::make_shared<ast::File>(filename_);
  antlr4::ANTLRInputStream inputStream(content_);
  walangLexer lexer(&inputStream);
  antlr4::CommonTokenStream tokens(&lexer);
  walangParser parser(&tokens);
  Listener listener(file);
  antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, parser.walang());
  return file;
}

} // namespace walang