#include "binaryen-c.h"
#include "compiler.hpp"
#include "fmt/color.h"
#include "fmt/core.h"
#include "parser.hpp"
#include <algorithm>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <list>
#include <stdexcept>

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
  bool optimize = false;

  std::list<std::string> arguments{};
  for (int i = 1; i < argc; i++) {
    arguments.emplace_back(argv[i]);
  }
  if (std::count(arguments.cbegin(), arguments.cend(), "-o") > 1) {
    printHelpAndExit();
  }
  auto outputIt = std::find(arguments.cbegin(), arguments.cend(), "-o");
  if (outputIt != arguments.end()) {
    outputIt = arguments.erase(outputIt);
    if (outputIt != arguments.end()) {
      outputFilePath = *outputIt;
      arguments.erase(outputIt);
    } else {
      printHelpAndExit();
    }
  }
  auto optimizeIt = std::find(arguments.cbegin(), arguments.cend(), "-O2");
  if (optimizeIt != arguments.end()) {
    optimize = true;
    arguments.erase(optimizeIt);
  }

  if (arguments.size() != 1) {
    printHelpAndExit();
  }
  inputFilePath = arguments.front();
  if (outputFilePath.empty()) {
    outputFilePath = std::filesystem::path{inputFilePath}.replace_extension("wat").string();
  }
  std::string source = readFile(inputFilePath);
  walang::FileParser parser(inputFilePath, source);
  auto file = parser.parse();
  walang::Compiler compiler({file});
  try {
    compiler.compile();
  } catch (std::exception const &e) {
    std::cerr << fmt::format("Compile Failed:\n{}", fmt::styled(e.what(), fmt::fg(fmt::color::orange))) << "\n";
    std::exit(-1);
  }
  std::ofstream outputFile{outputFilePath};
  if (!outputFile.is_open()) {
    std::cerr << "output path invalid " << outputFilePath << std::endl;
  }
  if (optimize) {
    BinaryenModuleValidate(compiler.module());
    BinaryenModuleOptimize(compiler.module());
    outputFile << compiler.wat();
  } else {
    outputFile << compiler.wat();
    BinaryenModuleValidate(compiler.module());
  }
}