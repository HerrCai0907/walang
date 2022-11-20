#pragma once

#include "generated/walangParser.h"
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>

namespace walang {
namespace ast {

class Node {
public:
  virtual ~Node() = default;
  virtual std::string to_string() = 0;
};

} // namespace ast
} // namespace walang