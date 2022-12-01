#include "fmt/format.h"
#include "generated/walangParser.h"
#include "statement.hpp"
#include <cassert>
#include <fmt/core.h>
#include <iterator>
#include <memory>
#include <vector>

namespace walang::ast {

ClassStatement::ClassStatement(walangParser::ClassStatementContext *ctx,
                               std::unordered_map<antlr4::ParserRuleContext *, std::shared_ptr<Node>> const &map)
    : Statement(StatementType::TypeClassStatement) {
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
  std::vector<std::string> memberStrings{};
  memberStrings.reserve(members_.size());
for (Member const &member : members_) {
    memberStrings.push_back(fmt::format("{0}:{1}\n", member.name_, member.type_));
  }
  std::vector<std::string> functionStrings{};
  functionStrings.reserve(functions_.size());
  for (std::shared_ptr<FunctionStatement> const &func : functions_) {
    functionStrings.push_back(func->to_string());
  }

  return fmt::format("class {0} {{\n{1}{2}}}\n", name_, fmt::join(memberStrings, ""), fmt::join(functionStrings, ""));
}

} // namespace walang::ast