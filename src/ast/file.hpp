#pragma once

#include "node.hpp"
#include "statement.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace walang::ast {

class File : public Node {
public:
  explicit File(std::string filename) : filename_(std::move(filename)) {}
  void update(walangParser::WalangContext *ctx,
              std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map);
  [[nodiscard]] std::string to_string() const override;
  [[nodiscard]] std::vector<std::shared_ptr<Statement>> const &statement() const noexcept { return statements_; }
  [[nodiscard]] std::string const &filename() const noexcept { return filename_; }

private:
  std::string filename_;
  std::vector<std::shared_ptr<Statement>> statements_;
};

} // namespace walang::ast
