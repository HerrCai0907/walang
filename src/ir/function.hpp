#pragma once

#include "ast/statement.hpp"
#include "binaryen-c.h"
#include "ir/variant.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace walang {
namespace ir {

class Function {
public:
  explicit Function(std::string_view name) : name_(name) {}

  std::string name() const noexcept { return name_; }

  std::shared_ptr<Local> &addTempLocal() { return locals_.emplace_back(std::make_shared<Local>(locals_.size())); }

  BinaryenFunctionRef finialize(BinaryenModuleRef module, BinaryenExpressionRef body) {
    std::vector<BinaryenType> types{};
    for (auto const &local : locals_) {
      types.emplace_back(BinaryenTypeInt32());
    }
    return BinaryenAddFunction(module, name_.c_str(), BinaryenTypeNone(), BinaryenTypeNone(), types.data(),
                               types.size(), body);
  }

private:
  std::string name_;
  std::vector<std::shared_ptr<Local>> locals_;
};

} // namespace ir
} // namespace walang
