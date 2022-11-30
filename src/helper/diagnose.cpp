#include "diagnose.hpp"
#include "fmt/core.h"
#include <stdexcept>

namespace walang {

TypeConvertError::TypeConvertError(std::shared_ptr<ir::VariantType const> const &from,
                                   std::shared_ptr<ir::VariantType const> const &to)
    : CompilerError(), from_(from), to_(to) {}
TypeConvertError::TypeConvertError(std::shared_ptr<ir::VariantType const> const &from,
                                   std::shared_ptr<ir::VariantType const> const &to, ast::Range const &range)
    : CompilerError(range), from_(from), to_(to) {
  generateErrorMessage();
}
void TypeConvertError::generateErrorMessage() {
  errorMessage_ = fmt::format("invalid convert from {0} to {1}\n\t{2}", from_->to_string(), to_->to_string(), range_);
}

const char *TypeConvertError::what() const noexcept { return errorMessage_.c_str(); }

} // namespace walang