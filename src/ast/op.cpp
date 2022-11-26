#include "op.hpp"
#include "generated/walangParser.h"
#include "tree/TerminalNode.h"
#include <iostream>
#include <magic_enum.hpp>

namespace walang {
namespace ast {

BinaryOp Operator::getOp(walangParser::BinaryOperatorContext *ctx) noexcept {
  if (ctx->Plus() != nullptr) {
    return BinaryOp::ADD;
  } else if (ctx->Minus() != nullptr) {
    return BinaryOp::SUB;
  } else if (ctx->Star() != nullptr) {
    return BinaryOp::MUL;
  } else if (ctx->Div() != nullptr) {
    return BinaryOp::DIV;
  } else if (ctx->Mod() != nullptr) {
    return BinaryOp::MOD;
  } else if (ctx->LeftShift() != nullptr) {
    return BinaryOp::LEFT_SHIFT;
  } else if (ctx->RightShift() != nullptr) {
    return BinaryOp::RIGHT_SHIFT;
  } else if (ctx->Less() != nullptr) {
    return BinaryOp::LESS_THAN;
  } else if (ctx->Greater() != nullptr) {
    return BinaryOp::GREATER_THAN;
  } else if (ctx->LessEqual() != nullptr) {
    return BinaryOp::NO_GREATER_THAN;
  } else if (ctx->GreaterEqual() != nullptr) {
    return BinaryOp::NO_LESS_THAN;
  } else if (ctx->Equal() != nullptr) {
    return BinaryOp::EQUAL;
  } else if (ctx->NotEqual() != nullptr) {
    return BinaryOp::NOT_EQUAL;
  } else if (ctx->And() != nullptr) {
    return BinaryOp::AND;
  } else if (ctx->Caret() != nullptr) {
    return BinaryOp::XOR;
  } else if (ctx->Or() != nullptr) {
    return BinaryOp::OR;
  } else if (ctx->AndAnd() != nullptr) {
    return BinaryOp::LOGIC_AND;
  } else if (ctx->OrOr() != nullptr) {
    return BinaryOp::LOGIC_OR;
  }
  std::cerr << "unknown operator " << ctx->getText() << std::endl;
  std::terminate();
}

PrefixOp Operator::getOp(walangParser::PrefixOperatorContext *ctx) noexcept {
  if (ctx->NOT() != nullptr) {
    return PrefixOp::NOT;
  } else if (ctx->Plus() != nullptr) {
    return PrefixOp::ADD;
  } else if (ctx->Minus() != nullptr) {
    return PrefixOp::SUB;
  }
  std::cerr << "unknown operator " << ctx->getText() << std::endl;
  std::terminate();
}

int Operator::getOpPriority(BinaryOp op) noexcept {
  switch (op) {
  case BinaryOp::MUL:
  case BinaryOp::DIV:
  case BinaryOp::MOD:
    return 3;
  case BinaryOp::ADD:
  case BinaryOp::SUB:
    return 4;
  case BinaryOp::LEFT_SHIFT:
  case BinaryOp::RIGHT_SHIFT:
    return 5;
  case BinaryOp::LESS_THAN:
  case BinaryOp::GREATER_THAN:
  case BinaryOp::NO_LESS_THAN:
  case BinaryOp::NO_GREATER_THAN:
    return 6;
  case BinaryOp::EQUAL:
  case BinaryOp::NOT_EQUAL:
    return 7;
  case BinaryOp::AND:
    return 8;
  case BinaryOp::XOR:
    return 9;
  case BinaryOp::OR:
    return 10;
  case BinaryOp::LOGIC_AND:
    return 11;
  case BinaryOp::LOGIC_OR:
    return 12;
  }
  std::cerr << "unknown operator " << magic_enum::enum_integer(op) << std::endl;
  std::terminate();
}

std::string Operator::to_string(BinaryOp op) { return std::string{magic_enum::enum_name(op)}; }
std::string Operator::to_string(PrefixOp op) { return std::string{magic_enum::enum_name(op)}; }

} // namespace ast
} // namespace walang
