#include "fmt/format.h"
#include "generated/walangParser.h"
#include "statement.hpp"
#include <algorithm>
#include <cassert>
#include <fmt/core.h>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace walang {
namespace ast {

ClassStatement::ClassStatement(walangParser::ClassStatementContext *ctx,
                               std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map)
    : Statement(Type::_ClassStatement) {
  name_ = ctx->Identifier()->getText();

  for (walangParser::MemberContext *memberCtx : ctx->member()) {
    members_.push_back(Member{memberCtx->Identifier()->getText(), memberCtx->type()->getText()});
  }
  for (walangParser::FunctionStatementContext *functionCtx : ctx->functionStatement()) {
    assert(map.count(functionCtx) == 1);
    functions_.push_back(std::dynamic_pointer_cast<FunctionStatement>(map.find(functionCtx)->second));
  }
}
std::string ClassStatement::to_string() const {
  std::vector<std::string> memberStrs{};
  for (Member const &member : members_) {
    memberStrs.push_back(fmt::format("{0}:{1}\n", member.name_, member.type_));
  }
  std::vector<std::string> functionStrs{};
  for (std::shared_ptr<FunctionStatement> const &func : functions_) {
    functionStrs.push_back(func->to_string());
  }

  return fmt::format("class {0} {{\n{1}{2}}}\n", name_, fmt::join(memberStrs, ""), fmt::join(functionStrs, ""));
}

} // namespace ast
} // namespace walang