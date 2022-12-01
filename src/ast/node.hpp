#pragma once

#include "generated/walangParser.h"
#include "helper/range.hpp"
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>

namespace walang {
namespace ast {

class Node {
public:
  virtual ~Node() = default;
  virtual std::string to_string() const = 0;

  void setRange(std::shared_ptr<File> const &file, antlr4::ParserRuleContext *ctx) { range_ = Range{file, ctx}; }
  Range const &range() { return range_; }

protected:
  Range range_;
};

} // namespace ast
} // namespace walang