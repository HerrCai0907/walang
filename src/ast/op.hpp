#pragma once

#include "generated/walangParser.h"
#include <string>

namespace walang {
namespace ast {

enum class Op {
  ADD = 1,
  SUB,
  MUL,
  DIV,
  MOD,
  LEFT_SHIFT,
  RIGHT_SHIFT,
  LESS_THAN,
  GREATER_THAN,
  NO_LESS_THAN,
  NO_GREATER_THAN,
  EQUAL,
  NOT_EQUAL,
  AND,
  OR,
  XOR,
  LOGIC_AND,
  LOGIC_OR,
  MEMBER,
};

class Operator {
public:
  static Op getOp(walangParser::BinaryOperatorContext *ctx) noexcept;
  static int getOpPriority(Op op) noexcept;
  static std::string to_string(Op op);
};

} // namespace ast
} // namespace walang