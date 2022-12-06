#pragma once

#include "helper/diagnose.hpp"
#include <set>
#include <string>

namespace walang {

class RedefinedChecker {
public:
  void check(std::string const &name) {
    if (set_.count(name)) {
      throw RedefinedSymbol(name);
    } else {
      set_.insert(name);
    }
  }

private:
  std::set<std::string> set_{};
};

} // namespace walang
