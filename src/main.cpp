#include "parser.hpp"

int main() {
  walang::FileParser parser("t.wa", R"(
let a = 1;
a = a + 4 * 1;
)");
  parser.parse();
}