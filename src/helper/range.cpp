#include "range.hpp"
#include "ast/file.hpp"
#include <fmt/core.h>

namespace walang::ast {

Range::Range(std::shared_ptr<File> const &file, antlr4::ParserRuleContext *ctx)
    : file_(file),
      start_(Position{.line = ctx->getStart()->getLine(), .column = ctx->getStart()->getCharPositionInLine()}),
      end_(Position{.line = ctx->getStop()->getLine(),
                    .column = ctx->getStop()->getCharPositionInLine() + 1U + ctx->getStop()->getStopIndex() -
                              ctx->getStop()->getStartIndex()}) {}

std::string Range::to_string() const {
  // vscode use 1 base column
  auto file = file_.lock();
  return fmt::format("{0}:{1}:{2} - {0}:{3}:{4}", file ? file->filename() : "unknown file", start_.line,
                     start_.column + 1, end_.line, end_.column + 1);
}

} // namespace walang::ast