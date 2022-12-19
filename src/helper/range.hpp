#pragma once

#include "ParserRuleContext.h"
#include <cstdint>
#include <fmt/core.h>
#include <fmt/format.h>
#include <memory>
#include <string_view>

namespace walang::ast {

class File;

struct Position {
  size_t line;
  size_t column;
};

class Range {
public:
  Range() = default;
  Range(std::shared_ptr<File> const &file, antlr4::ParserRuleContext *ctx);

  [[nodiscard]] std::string to_string() const;
  [[nodiscard]] Position const &start() const noexcept { return start_; }
  [[nodiscard]] Position const &end() const noexcept { return end_; }

private:
  std::weak_ptr<File> file_;
  Position start_{};
  Position end_{};
};

} // namespace walang::ast

template <> struct fmt::formatter<walang::ast::Range> : fmt::formatter<std::string_view> {
  // parse is inherited from formatter<string_view>.
  template <typename FormatContext> auto format(walang::ast::Range const &range, FormatContext &ctx) const {
    return formatter<string_view>::format(range.to_string(), ctx);
  }
};
