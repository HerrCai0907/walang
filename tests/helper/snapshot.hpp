#pragma once

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tinyxml2.h>
#include <vector>

namespace test_helper {

class SnapShot {
public:
  explicit SnapShot(std::filesystem::path const &filename) : filename_(filename) {
    if (std::filesystem::is_regular_file(filename)) {
      auto err = doc.LoadFile(std::filesystem::absolute(filename_).c_str());
      if (err == tinyxml2::XML_SUCCESS) {
        auto child = doc.FirstChild()->ToElement()->FirstChildElement();
        while (child != nullptr) {
          map_.emplace(child->Name(), child);
          child = child->NextSiblingElement();
        }
        return;
      }
    }
    if (isUpdate()) {
      doc.Parse("<snapshots></snapshots>");
    } else {
      std::cerr << "cannot parser xml" << std::endl;
      std::exit(-1);
    }
  }
  void check(std::string_view value) {
    std::string key{testing::UnitTest::GetInstance()->current_test_info()->name()};
    std::string text{value};
    text = "\n" + text;
    auto element = map_.find(key);
    if (element == map_.end()) {
      std::cout << "add snapshot " << key << std::endl;
      doc.FirstChild()->ToElement()->InsertNewChildElement(key.data())->InsertNewText(text.data());
      auto err = doc.SaveFile(std::filesystem::absolute(filename_).c_str());
      assert(err == tinyxml2::XML_SUCCESS);
    } else {
      checked_.insert(key);
      if (isUpdate()) {
        if (element->second->GetText() == nullptr || element->second->GetText() != text) {
          std::cout << "update snapshot " << key << std::endl;
          element->second->SetText(text.data());
          auto err = doc.SaveFile(std::filesystem::absolute(filename_).c_str());
          assert(err == tinyxml2::XML_SUCCESS);
        }
      } else {
        ASSERT_NE(element->second->GetText(), nullptr);
        ASSERT_EQ(element->second->GetText(), text);
      }
    }
  }

private:
  std::filesystem::path filename_;
  tinyxml2::XMLDocument doc{};
  std::map<std::string, tinyxml2::XMLElement *> map_;
  std::set<std::string> checked_;

  static bool isUpdate() {
    auto env = getenv("UPDATE_SNAPSHOT");
    return env && env == std::string("yes");
  }
};

} // namespace test_helper