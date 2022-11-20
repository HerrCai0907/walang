#include "ast/file.hpp"
#include "compiler.hpp"
#include "parser.hpp"
#include <vector>

int main() {
  walang::FileParser parser("t.wa", R"(
let a = 1 * 10;
a = a + 4 * 1;
)");
  auto file = parser.parse();
  walang::Compiler compiler({file});
  compiler.compile();
  compiler.dumpModule();
}