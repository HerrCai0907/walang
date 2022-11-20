#include "file.hpp"

namespace walang {
namespace ast {

void File::update(walangParser::WalangContext *ctx,
                  std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map) {
  std::vector<walangParser::StatementContext *> statements = ctx->statement();
  for (auto statement : statements) {
    antlr4::ParserRuleContext *child = nullptr;
    if (statement->assignStatement() != nullptr) {
      child = statement->assignStatement();
    } else if (statement->declareStatement() != nullptr) {
      child = statement->declareStatement();
    }
    assert(map.count(child) == 1);
    statements_.push_back(std::dynamic_pointer_cast<ast::Statement>(map.find(child)->second));
  }
}

std::string File::to_string() const {
  std::string str{};
  for (auto const &statement : statements_) {
    str += statement->to_string();
  }
  return str;
}

} // namespace ast
} // namespace walang