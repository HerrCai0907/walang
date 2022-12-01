#pragma once

#include "node.hpp"
#include "statement.hpp"
#include <memory>
#include <vector>

namespace walang {
namespace ast {

class File : public Node {
public:
  explicit File(std::string const &filename) : filename_(filename) {}
  void update(walangParser::WalangContext *ctx,
              std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  virtual std::string to_string() const override;
  std::vector<std::shared_ptr<Statement>> const &statement() const noexcept { return statements_; }
  std::string const &filename() const noexcept { return filename_; }

private:
  std::string filename_;
  std::vector<std::shared_ptr<Statement>> statements_;
};

} // namespace ast
} // namespace walang
