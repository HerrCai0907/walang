#include "op.hpp"
#include "generated/walangParser.h"
#include "tree/TerminalNode.h"
#include <iostream>
#include <magic_enum.hpp>

namespace walang {
namespace ast {

Op Operator::getOp(walangParser::BinaryOperatorContext *ctx) noexcept {
  if (ctx->Plus() != nullptr) {
    return Op::ADD;
  } else if (ctx->Minus() != nullptr) {
    return Op::SUB;
  } else if (ctx->Star() != nullptr) {
    return Op::MUL;
  } else if (ctx->Div() != nullptr) {
    return Op::DIV;
  } else if (ctx->Mod() != nullptr) {
    return Op::MOD;
  } else if (ctx->LeftShift() != nullptr) {
    return Op::LEFT_SHIFT;
  } else if (ctx->RightShift() != nullptr) {
    return Op::RIGHT_SHIFT;
  } else if (ctx->Less() != nullptr) {
    return Op::LESS_THAN;
  } else if (ctx->Greater() != nullptr) {
    return Op::GREATER_THAN;
  } else if (ctx->LessEqual() != nullptr) {
    return Op::NO_GREATER_THAN;
  } else if (ctx->GreaterEqual() != nullptr) {
    return Op::NO_LESS_THAN;
  } else if (ctx->Equal() != nullptr) {
    return Op::EQUAL;
  } else if (ctx->NotEqual() != nullptr) {
    return Op::NOT_EQUAL;
  } else if (ctx->And() != nullptr) {
    return Op::AND;
  } else if (ctx->Caret() != nullptr) {
    return Op::XOR;
  } else if (ctx->Or() != nullptr) {
    return Op::OR;
  } else if (ctx->AndAnd() != nullptr) {
    return Op::LOGIC_AND;
  } else if (ctx->OrOr() != nullptr) {
    return Op::LOGIC_OR;
  } else if (ctx->Dot() != nullptr) {
    return Op::MEMBER;
  }
  std::cerr << "unknown operator " << ctx->getText() << std::endl;
  std::terminate();
}
int Operator::getOpPriority(Op op) noexcept {
  switch (op) {
  case Op::MEMBER:
    return 1;
  case Op::MUL:
  case Op::DIV:
  case Op::MOD:
    return 3;
  case Op::ADD:
  case Op::SUB:
    return 4;
  case Op::LEFT_SHIFT:
  case Op::RIGHT_SHIFT:
    return 5;
  case Op::LESS_THAN:
  case Op::GREATER_THAN:
  case Op::NO_LESS_THAN:
  case Op::NO_GREATER_THAN:
    return 6;
  case Op::EQUAL:
  case Op::NOT_EQUAL:
    return 7;
  case Op::AND:
    return 8;
  case Op::XOR:
    return 9;
  case Op::OR:
    return 10;
  case Op::LOGIC_AND:
    return 11;
  case Op::LOGIC_OR:
    return 12;
  }
  std::cerr << "unknown operator " << magic_enum::enum_integer(op) << std::endl;
  std::terminate();
}
std::string Operator::to_string(Op op) { return std::string{magic_enum::enum_name(op)}; }

} // namespace ast
} // namespace walang
