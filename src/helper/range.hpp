#pragma once

#include "ParserRuleContext.h"
#include "fmt/core.h"
#include "fmt/format.h"
#include <cstdint>
#include <memory>
#include <string_view>

namespace walang {
namespace ast {

class File;

struct Position {
  size_t line;
  size_t column;
};

class Range {
public:
  Range() {}
  Range(std::shared_ptr<File> const &file, antlr4::ParserRuleContext *ctx);
  std::string to_string() const;

private:
  std::weak_ptr<File> file_;
  Position start_;
  Position end_;
};

} // namespace ast
} // namespace walang

template <> struct fmt::formatter<walang::ast::Range> : fmt::formatter<std::string_view> {
  // parse is inherited from formatter<string_view>.
  template <typename FormatContext> auto format(walang::ast::Range const &range, FormatContext &ctx) const {
    return formatter<string_view>::format(range.to_string(), ctx);
  }
};
