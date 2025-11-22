#include "commands.h" // 包含命令处理函数的声明
#include "debug.h"    // 包含调试工具
#include <filesystem> // 如果使用文件系统操作
#include <fstream>    // 如果使用文件流
#include <iostream>   // 用于标准输入输出
#include <ostream>
#include <string> // 用于字符串处理
#include "git_clone.h"

int main(int argc, char *argv[]) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  std::cerr << "Logs from your program will appear here!\n";

  // Uncomment this block to pass first stage

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
    return ls_tree(argc, argv);
  } else if (std::string(argv[1]) == "write-tree") {
    // 获取当前工作目录
    std::string current_dir = std::filesystem::current_path().string();
    // 递归生成当前目录的树对象
    std::string root_tree_hash = GenerateTreeObjectForDirectory(current_dir);
    std::cout << root_tree_hash << std::endl;
    return 0;
  } else if (std::string(argv[1]) == "commit-tree") {
    std::string tree_sha;
    std::string parent_commit_sha;
    std::string commit_message;
    // 遍历参数进行解析，而不是依赖固定位置
    for (int i = 2; i < argc; i++) {
      std::string arg = argv[i];

      if (arg == "-p" && i + 1 < argc) {
        parent_commit_sha = argv[++i]; // 获取父提交哈希
      } else if (arg == "-m" && i + 1 < argc) {
        commit_message = argv[++i]; // 获取提交信息
      } else {
        // 假设第一个非选项参数是树对象哈希
        if (tree_sha.empty()) {
          tree_sha = arg;
        }
      }
    }
    std::string sha;
    if (parent_commit_sha.empty()) {
      // 初始提交：没有父提交
      sha = WriteTreeWithInitialCommit(tree_sha, commit_message);
    } else {
      // 普通提交：有父提交
      sha = WriteTreeWithParentSHA(tree_sha, parent_commit_sha, commit_message);
    }
    std::cout << sha << std::endl;
    return 0;
  } else if (std::string(argv[1]) == "clone") {
    if (argc < 3) {
      std::cerr << "No repository provided.\n";
      return EXIT_FAILURE;
    }
    std::string url = argv[2];
    std::string directory = argv[3];
    if (clone(url, directory) != EXIT_SUCCESS) {
      std::cerr << "Failed to clone repository.\n";
      return EXIT_FAILURE;
    }
  } else {
    std::cerr << "Unknown command " << command << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
