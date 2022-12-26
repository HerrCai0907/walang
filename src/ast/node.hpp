#pragma once

#include "generated/walangParser.h"
#include "helper/range.hpp"
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>

namespace walang::ast {

class Node {
public:
  virtual ~Node() = default;
  [[nodiscard]] virtual std::string to_string() const = 0;

  void setRange(std::shared_ptr<File> const &file, antlr4::ParserRuleContext *ctx) { range_ = Range{file, ctx}; }
  [[nodiscard]] Range const &range() const { return range_; }

protected:
  Range range_;
};

} // namespace walang::ast