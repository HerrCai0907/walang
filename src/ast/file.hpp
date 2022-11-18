#pragma once

#include "statement.hpp"
#include <memory>
#include <vector>

namespace walang {
namespace ast {

class File {
public:
private:
  std::vector<std::shared_ptr<Statement>> statements_;
};

} // namespace ast
} // namespace walang
