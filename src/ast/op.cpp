#include "op.hpp"
#include <iostream>
#include <magic_enum.hpp>

namespace walang {
namespace ast {

Op Operator::getOp(walangParser::BinaryOperatorContext *ctx) noexcept {
  if (ctx->Plus() != nullptr) {
    return Op::ADD;
  } else if (ctx->Star() != nullptr) {
    return Op::MUL;
  }
  std::cerr << "unknown operator " << ctx->getText() << std::endl;
  std::terminate();
}
int Operator::getOpPriority(Op op) noexcept {
  switch (op) {
  case Op::ADD:
    return 1;
  case Op::MUL:
    return 2;
  }
  std::cerr << "unknown operator " << magic_enum::enum_integer(op) << std::endl;
  std::terminate();
}
std::string Operator::to_string(Op op) { return std::string{magic_enum::enum_name(op)}; }

} // namespace ast
} // namespace walang
