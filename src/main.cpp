#include "commands.h" // 包含命令处理函数的声明
#include "commands.h"
#include "debug.h"    // 包含调试工具
#include <filesystem> // 如果使用文件系统操作
#include <fstream>    // 如果使用文件流
#include <iostream>   // 用于标准输入输出
#include <iterator>
#include <stdexcept> // 用于异常处理
#include <string>    // 用于字符串处理
#include <vector>

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
    std::string blob_content = GetBlobContentFromInputHash(hash);
    std::cout << blob_content << std::flush;
    return 0;
  } else if (argc == 4 && std::string(argv[1]) == "hash-object" &&
             std::string(argv[2]) == "-w") {
    std::string file_name = argv[3]; // test.txt
    auto sha1_hash = GenerateBlobObjectForFile(file_name);
    std::cout << sha1_hash << std::flush;
    return 0;
  } else if ((std::string(argv[1]) == "ls-tree")) {
    // ls-tree --name-only <tree_sha>
    return HandleLsTreeCommand(argc, argv);
  } else {
    std::cerr << "Unknown command " << command << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}