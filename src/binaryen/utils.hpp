#pragma once

#include <binaryen-c.h>
#include <vector>

namespace walang::binaryen {

class Utils {
public:
  static BinaryenExpressionRef combineExprRef(BinaryenModuleRef module, std::vector<BinaryenExpressionRef> &vec) {
    return vec.size() == 1 ? vec.back() : BinaryenBlock(module, nullptr, vec.data(), vec.size(), BinaryenTypeAuto());
  }
};

} // namespace walang::binaryen
