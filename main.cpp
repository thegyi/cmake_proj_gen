#include <algorithm>
#include <boost/program_options.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

std::string replace(std::string from, const std::string_view oldValue,
                    const std::string_view newValue) {

  auto it = from.find(oldValue);
  if (it == std::string::npos) {
    return from;
  }
  return from.replace(it, oldValue.size(), newValue);
}

void replaceAll(
    const std::filesystem::path &inFile, const std::filesystem::path &outFile,
    const std::vector<std::pair<std::string, std::string>> &values) {
  std::ifstream in(inFile);
  std::string str((std::istreambuf_iterator<char>(in)),
                  std::istreambuf_iterator<char>());
  in.close();
  for (const auto [oldValue, newValue] : values) {
    str = replace(str, oldValue, newValue);
  }
  std::ofstream os(outFile);
  os << str;
  os.close();
}

int main(int argc, char const *argv[]) {

  std::string standard = "17";
  std::string projectType = "executable";
  std::string projectName = "myProject";
  std::string targetDir = ".";
  std::string templateDir = "/etc/cmake_project_generator/templates";

  boost::program_options::options_description desc("Allowed options");
  desc.add_options()("help", "produce help message")(
      "standard,s",
      boost::program_options::value<std::string>(&standard)->default_value(
          "17"),
      "c++ standard (11,17,20,23)")(
      "name,n", boost::program_options::value<std::string>(),
      "name of the project")(
      "type,t",
      boost::program_options::value<std::string>(&projectType)
          ->default_value("executable"),
      "type of the project (executable, static_lib, shared_lib)")(
      "output,o",
      boost::program_options::value<std::string>(&targetDir)
          ->default_value("."),
      "target directory");
  boost::program_options::variables_map vm;
  try {
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc), vm);
  } catch (
      boost::wrapexcept<boost::program_options::invalid_command_line_syntax>
          &ex) {
    std::cerr << "invalid commandline parameters:" << ex.what() << std::endl;
  }

  if (vm.count("help")) {
    std::cout << "Usage: options_description [options]\n";
    std::cout << desc;
    return 0;
  }

  if (vm.count("output")) {
    targetDir = vm["output"].as<std::string>();
  }
  if (vm.count("standard")) {
    standard = vm["standard"].as<std::string>();
  }
  if (vm.count("name")) {
    projectName = vm["name"].as<std::string>();
  }
  if (vm.count("type")) {
    projectType = vm["type"].as<std::string>();
  }

  std::filesystem::path targetDirPath(targetDir);
  if (!std::filesystem::exists(targetDir)) {
    std::filesystem::create_directory(targetDirPath);
  }
  std::filesystem::path targetSourceDirPath(targetDir + "/src");
  if (!std::filesystem::exists(targetSourceDirPath)) {
    std::filesystem::create_directory(targetSourceDirPath);
  }
  std::filesystem::path targetIncludeDirPath(targetDir + "/include");
  if (!std::filesystem::exists(targetIncludeDirPath)) {
    std::filesystem::create_directory(targetIncludeDirPath);
  }

  std::filesystem::path targetCMakeListsPath(targetDir + "/CMakeLists.txt");
  std::vector<std::pair<std::string, std::string>> templateValues;
  templateValues.emplace_back("%PROJECT_NAME%", projectName);
  templateValues.emplace_back("%CXX_STANDARD%", standard);
  replaceAll(templateDir + "/" + projectType + "/CMakeLists.txt",
             targetCMakeListsPath, templateValues);

  if (projectType == "executable") {
    std::filesystem::copy(templateDir + "/" + projectType + "/" + "main.cpp",
                          targetSourceDirPath,
                          std::filesystem::copy_options::overwrite_existing);
  } else if (projectType == "static_lib" || projectType == "shared_lib") {
    std::filesystem::copy(templateDir + "/" + projectType + "/" + "library.cpp",
                          targetSourceDirPath,
                          std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy(templateDir + "/" + projectType + "/" + "library.h",
                          targetIncludeDirPath,
                          std::filesystem::copy_options::overwrite_existing);
  }

  return 0;
}
