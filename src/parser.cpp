#include "parser.hpp"
#include "ANTLRInputStream.h"
#include "CommonTokenStream.h"
#include "generated/walangBaseListener.h"
#include "generated/walangLexer.h"
#include "generated/walangParser.h"
#include "tree/ParseTreeWalker.h"
#include <iostream>
#include <map>

namespace walang {

class Listener : public walangBaseListener {
public:
  virtual void exitIdentifier(walangParser::IdentifierContext *ctx) override {
    std::cout << "(id " << ctx->getText() << ")";
  }

  virtual void exitStatement(walangParser::StatementContext *) override {
    std::cout << "\n";
  }

  virtual void visitErrorNode(antlr4::tree::ErrorNode *node) override {
    std::cerr << "unexcepted " << node->getText() << std::endl;
    std::terminate();
  }
};

void FileParser::parse() {
  antlr4::ANTLRInputStream inputStream(content_);
  walangLexer lexer(&inputStream);
  antlr4::CommonTokenStream tokens(&lexer);
  walangParser parser(&tokens);
  Listener listener;
  antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, parser.walang());
}

} // namespace walang