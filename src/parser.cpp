#include "parser.hpp"
#include "ANTLRInputStream.h"
#include "CommonTokenStream.h"
#include "ast/expression.hpp"
#include "ast/file.hpp"
#include "ast/statement.hpp"
#include "generated/walangBaseListener.h"
#include "generated/walangLexer.h"
#include "generated/walangParser.h"
#include "tree/ParseTreeWalker.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

namespace walang {

class Listener : public walangBaseListener {
public:
  explicit Listener(std::shared_ptr<ast::File> file) : file_{file} {}

  virtual void exitWalang(walangParser::WalangContext *ctx) override { file_->update(ctx, astNodes_); }

  virtual void exitDeclareStatement(walangParser::DeclareStatementContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::DeclareStatement>(ctx, astNodes_));
  }
  virtual void exitAssignStatement(walangParser::AssignStatementContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::AssignStatement>(ctx, astNodes_));
  }
  virtual void exitExpressionStatement(walangParser::ExpressionStatementContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::ExpressionStatement>(ctx, astNodes_));
  }

  virtual void exitExpression(walangParser::ExpressionContext *ctx) override {
    antlr4::ParserRuleContext *child = nullptr;
    if (ctx->identifier() != nullptr) {
      child = ctx->identifier();
    } else if (ctx->binaryExpression() != nullptr) {
      child = ctx->binaryExpression();
    } else if (ctx->prefixExpression() != nullptr) {
      child = ctx->prefixExpression();
    }
    assert(astNodes_.count(child) == 1);
    astNodes_.emplace(ctx, astNodes_.find(child)->second);
  }
  virtual void exitIdentifier(walangParser::IdentifierContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::Identifier>(ctx, astNodes_));
  }
  virtual void exitBinaryExpression(walangParser::BinaryExpressionContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::BinaryExpression>(ctx, astNodes_));
  }
  virtual void exitPrefixExpression(walangParser::PrefixExpressionContext *ctx) override {
    astNodes_.emplace(ctx, std::make_shared<ast::PrefixExpression>(ctx, astNodes_));
  }

  virtual void visitErrorNode(antlr4::tree::ErrorNode *node) override {
    std::cerr << "unexcepted " << node->getText() << std::endl;
    std::terminate();
  }

private:
  std::shared_ptr<ast::File> file_;
  std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> astNodes_;
};

std::shared_ptr<ast::File> FileParser::parse() {
  auto file = std::make_shared<ast::File>();
  antlr4::ANTLRInputStream inputStream(content_);
  walangLexer lexer(&inputStream);
  antlr4::CommonTokenStream tokens(&lexer);
  walangParser parser(&tokens);
  Listener listener(file);
  antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, parser.walang());
  return file;
}

} // namespace walang