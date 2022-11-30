#include "ast/file.hpp"
#include "compiler.hpp"
#include "parser.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

[[noreturn]] void printHelpAndExit() {
  std::cerr << "walang source [-o target])" << std::endl;
  std::exit(-1);
}

std::string readFile(std::string const &path) {
  std::ifstream file{path, std::ios::binary};
  if (!file.is_open()) {
    throw std::runtime_error(std::string("invalid path") + std::string(path));
  }
  std::ostringstream tmp;
  tmp << file.rdbuf();
  return tmp.str();
}

int main(int argc, const char *argv[]) {
  std::string inputFilePath;
  std::string outputFilePath;

  std::list<std::string> arguments{};
  for (int i = 1; i < argc; i++) {
    arguments.emplace_back(argv[i]);
  }
  if (std::count(arguments.cbegin(), arguments.cend(), "-o") > 1) {
    printHelpAndExit();
  }
  auto it = std::find(arguments.cbegin(), arguments.cend(), "-o");
  if (it != arguments.end()) {
    it = arguments.erase(it);
    if (it != arguments.end()) {
      outputFilePath = *it;
      arguments.erase(it);
    } else {
      printHelpAndExit();
    }
  }
  if (arguments.size() != 1) {
    printHelpAndExit();
  }
  inputFilePath = arguments.front();
  if (outputFilePath == "") {
    outputFilePath = std::filesystem::path{inputFilePath}.replace_extension("wat").string();
  }
  std::string source = readFile(inputFilePath);
  walang::FileParser parser(inputFilePath, source);
  auto file = parser.parse();
  walang::Compiler compiler({file});
  compiler.compile();
  BinaryenModuleValidate(compiler.module());

  std::ofstream outputFile{outputFilePath};
  if (!outputFile.is_open()) {
    std::cerr << "output path invalid " << outputFilePath << std::endl;
  }
  outputFile << compiler.wat();
}