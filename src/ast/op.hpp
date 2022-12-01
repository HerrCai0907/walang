#pragma once

#include "generated/walangParser.h"
#include <string>
#include <variant>

namespace walang::ast {

enum class BinaryOp {
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
};

enum class PrefixOp {
  ADD = 1,
  SUB,
  NOT,
};

class Operator {
public:
  static BinaryOp getOp(walangParser::BinaryOperatorContext *ctx) noexcept;
  static PrefixOp getOp(walangParser::PrefixOperatorContext *ctx) noexcept;

  static int getOpPriority(BinaryOp op) noexcept;

  static std::string to_string(BinaryOp op);
  static std::string to_string(PrefixOp op);
  static std::string to_string(std::variant<PrefixOp, BinaryOp> op);
};

} // namespace walang