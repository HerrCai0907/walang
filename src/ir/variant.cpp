#include "variant.hpp"
#include <stdexcept>

namespace walang::ir {

std::vector<BinaryenExpressionRef> Variant::assignTo(BinaryenModuleRef module, Variant const *to) const {
  switch (to->type()) {
  case Symbol::Type::TypeGlobal:
    return assignToGlobal(module, *dynamic_cast<Global const *>(to));
  case Symbol::Type::TypeLocal:
    return assignToLocal(module, *dynamic_cast<Local const *>(to));
  case Symbol::Type::TypeMemoryData:
    return assignToMemory(module, *dynamic_cast<MemoryData const *>(to));
  case Symbol::Type::TypeStackData:
    return assignToStack(module);
  case Symbol::Type::TypeFunction:
    break;
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}

} // namespace walang::ir