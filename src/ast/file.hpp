#pragma once

#include "node.hpp"
#include "statement.hpp"
#include <memory>
#include <vector>

namespace walang {
namespace ast {

class File : public Node {
public:
  void update(walangParser::WalangContext *ctx,
              std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<ast::Node>> const &map);
  virtual std::string to_string() override;

private:
  std::vector<std::shared_ptr<Statement>> statements_;
};

} // namespace ast
} // namespace walang
