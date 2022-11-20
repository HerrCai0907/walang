#pragma once

#include "generated/walangParser.h"
#include <string>

namespace walang {
namespace ast {

enum class Op {
  ADD,
  MUL,
};

class Operator {
public:
  static Op getOp(walangParser::BinaryOperatorContext *ctx) noexcept;
  static int getOpPriority(Op op) noexcept;
  static std::string to_string(Op op);
};

} // namespace ast
} // namespace walang