#pragma once

#include "ast/statement.hpp"
#include <memory>
#include <string>
#include <vector>

namespace walang {
namespace ir {

class Global {
public:
  explicit Global(ast::DeclareStatement const &declare) : name_{declare.name()}, init_{declare.init()} {}

  std::string name() const noexcept { return name_; }

private:
  std::string name_;
  std::shared_ptr<ast::Expression> init_;
};

} // namespace ir
} // namespace walang
