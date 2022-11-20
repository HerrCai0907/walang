#pragma once

#include "ast/file.hpp"
#include <memory>
#include <string>
#include <string_view>

namespace walang {

class FileParser {
public:
  FileParser(std::string_view filename, std::string_view const &content) : filename_(filename), content_(content) {}

  std::shared_ptr<ast::File> parse();

private:
  std::string filename_;
  std::string content_;
};

} // namespace walang