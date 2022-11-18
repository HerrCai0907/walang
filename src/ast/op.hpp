#pragma once

#include <exception>
#include <iostream>
#include <string_view>

namespace walang {
namespace ast {

enum class Op {
  ADD,
  MUL,
};

class OpChecker {
public:
  static Op getOp(std::string_view str) noexcept {
    if (str == "+") {
      return Op::ADD;
    } else if (str == "*") {
      return Op::MUL;
    }
    std::cerr << "unknown operator " << str << std::endl;
    std::terminate();
  }
};

} // namespace ast
} // namespace walang