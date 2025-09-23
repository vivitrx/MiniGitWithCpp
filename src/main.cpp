#include "debug.h"
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <zlib.h>

int main(int argc, char *argv[]) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  std::cerr << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage

  if (argc < 2) {
    std::cerr << "No command provided.\n";
    return EXIT_FAILURE;
  }

  std::string command = argv[1];

  if (command == "init") {
    try {
      std::filesystem::create_directory(".git");
      std::filesystem::create_directory(".git/objects");
      std::filesystem::create_directory(".git/refs");
      std::ofstream headFile(".git/HEAD");
      if (headFile.is_open()) {
        headFile << "ref: refs/heads/main\n";
        headFile.close();
      } else {
        std::cerr << "Failed to create .git/HEAD file.\n";
        return EXIT_FAILURE;
      }
      std::cout << "Initialized git directory\n";
    } catch (const std::filesystem::filesystem_error &e) {
      std::cerr << e.what() << '\n';
      return EXIT_FAILURE;
    }
  } else if (argc == 4 && std::string(argv[1]) == "cat-file" &&
             std::string(argv[2]) == "-p") {
    auto hash = std::string(argv[3]);
    dbg(hash);
    auto path = ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
    dbg(path);
    std::ifstream file(path, std::ios::binary);
    if (!file) {
      throw std::runtime_error("object not found: " + path);
    }
    // 把 file 里的内容读入到一个string里
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string result = buffer.str();
    file.close();
    // 你已经拿到 file 了，现在用zlib库解压他
    // 目前你只用完成对blob类型的解析，所以不需要解析 file 的文件头
    const auto result_content = result.substr(result.find('\0') + 1);
    std::cout << result_content << std::flush;
    return 0;
  } else {
    std::cerr << "Unknown command " << command << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
