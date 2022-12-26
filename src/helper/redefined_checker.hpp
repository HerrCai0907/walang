#pragma once

#include "helper/diagnose.hpp"
#include <cstdint>
#include <set>
#include <stack>
#include <string>

namespace walang {

class RedefinedChecker {
public:
  void check(std::string const &name) {
    if (set_.count(name)) {
      throw RedefinedSymbol(name);
    }
    set_.insert(name);
  }

private:
  std::set<std::string> set_{};
};

} // namespace walang
