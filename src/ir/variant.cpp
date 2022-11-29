#include "variant.hpp"
#include "ir/variant_type.hpp"
#include <binaryen-c.h>

namespace walang {
namespace ir {

static std::shared_ptr<VariantType> getTypeFromDeclare(ast::DeclareStatement const &declare) {
  if (declare.type() == "") {
    return VariantType::inferType(declare.init());
  } else {
    return VariantType::resolveType(declare.type());
  }
}

Global::Global(ast::DeclareStatement const &declare)
    : Variant(Type::_Global, getTypeFromDeclare(declare)), name_{declare.name()}, init_{declare.init()} {}

BinaryenExpressionRef Global::makeAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) {
  return BinaryenGlobalSet(module, name_.c_str(), exprRef);
}

BinaryenExpressionRef Local::makeAssign(BinaryenModuleRef module, BinaryenExpressionRef exprRef) {
  return BinaryenLocalSet(module, index_, exprRef);
}

} // namespace ir
} // namespace walang