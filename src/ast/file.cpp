#include "file.hpp"

namespace walang::ast {

void File::update(walangParser::WalangContext *ctx,
                  std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map) {
  std::vector<walangParser::StatementContext *> statements = ctx->statement();
  for (auto statement : statements) {
    auto *child = dynamic_cast<antlr4::ParserRuleContext *>(statement->children.at(0));
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

} // namespace walang