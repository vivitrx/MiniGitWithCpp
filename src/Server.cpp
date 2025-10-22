#include "../include/clone_gadget.h"
#include "refs.h"
#include <algorithm>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <string.h>
#include <string>
#include <vector>
#include <zlib.h>

using namespace std;

int main(int argc, char *argv[]) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  // std::cout << "Logs from your program will appear here!\n";

  if (argc < 2) {
    std::cerr << "No command provided.\n";
    return EXIT_FAILURE;
  }
  MiniGitRef GitRefsSys;
  std::string command = argv[1];

  if (command == "init") {
    try {
      std::filesystem::create_directory(".git");
      std::filesystem::create_directory(".git/objects");
      GitRefsSys.Init();
      std::cout << "Initialized git directory\n";
    } catch (const std::filesystem::filesystem_error &e) {
      std::cerr << e.what() << '\n';
      return EXIT_FAILURE;
    }
  } else if (command == "cat-file") {
    if (argc <= 3) {
      cerr << "Invalid arguments, required `-p <blob_sha>`\n";
      return EXIT_FAILURE;
    }
    const string flag = argv[2];
    if (flag != "-p") {
      std::cerr << "Invalid flag for cat-file, expected `-p`\n";
      return EXIT_FAILURE;
    }
    const string value = argv[3];
    const string dir_name = value.substr(0, 2);
    const string blob_sha = value.substr(2);
    // not secure - technically. - can have LFI
    std::string path = ".git/objects/" + dir_name + "/" + blob_sha;
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
      std::cerr << "Failed to open object file?\n";
      return EXIT_FAILURE;
    }
    in.seekg(0, std::ios::end);
    int fileSize = in.tellg();
    in.seekg(0, std::ios::beg);
    if (fileSize > 0) {
      std::vector<Bytef> buffer(fileSize);
      in.read(reinterpret_cast<char *>(buffer.data()), fileSize);
      uLongf decompressedSize = fileSize * 100;
      std::vector<Bytef> decompressedData(decompressedSize);
      int result = uncompress(decompressedData.data(), &decompressedSize,
                              buffer.data(), fileSize);
      if (result == Z_OK) {
        bool isSignFound = false;
        for (int i = 0; i < decompressedSize; i++) {
          if (isSignFound)
            std::cout << decompressedData[i];
          if (decompressedData[i] == '\0') {
            isSignFound = true;
          }
        }
      }
    }
  } else if (command == "hash-object") {
    if (argc < 3) {
      std::cerr << "Less than 3 argument " << '\n';
      return EXIT_FAILURE;
    }
    std::string hash = argv[3];
    cout << hash_object(hash) << endl;
  }
  // The ls-tree command
  else if (command == "ls-tree") {
    std::string flag = "";
    std::string tree_sha = "";
    // if no argument is given
    if (argc <= 2) {
      std::cerr << "Invalid Arguments" << "\n";
      return EXIT_FAILURE;
    }
    // if the flag is not included
    if (argc == 3) {
      tree_sha = argv[2];
    } else {
      tree_sha = argv[3];
      flag = argv[2];
    }
    // if the flag is wrong
    if (flag.size() != 0 && flag != "--name-only") {
      std::cerr << "Incorrect flag :" << flag << "\nexpected --name-only"
                << "\n";
      return EXIT_FAILURE;
    }
    std::string dir_name = tree_sha.substr(0, 2);
    std::string file_name = tree_sha.substr(2);
    std::string path = "./.git/objects/" + dir_name + "/" + file_name;
    auto file = std::ifstream(path);
    if (!file.is_open()) {
      std::cerr << "Failed to open file: " << path << "\n";
      return EXIT_FAILURE;
    }
    auto tree_data = std::string(std::istreambuf_iterator<char>(file),
                                 std::istreambuf_iterator<char>());
    std::string buf;
    buf.resize(tree_data.size());
    while (true) {
      auto buf_size = buf.size();
      auto res = uncompress(reinterpret_cast<Bytef *>(buf.data()), &buf_size,
                            reinterpret_cast<const Bytef *>(tree_data.data()),
                            tree_data.size());
      if (res == Z_BUF_ERROR) {
        buf.resize(buf.size() * 2);
      } else if (res != Z_OK) {
        std::cerr << "Failed to decompress file, code:" << res << "\n";
        return EXIT_FAILURE;
      } else {
        buf.resize(buf_size);
        break;
      }
    }
    std::string trimmed_data = buf.substr(buf.find('\0') + 1);
    std::string line;
    std::vector<std::string> names;
    do {

      line = trimmed_data.substr(0, trimmed_data.find('\0'));
      if (line.substr(0, 5) == "40000")
        names.push_back(line.substr(6));
      else
        names.push_back(line.substr(7));
      trimmed_data = trimmed_data.substr(trimmed_data.find('\0') + 21);
    } while (trimmed_data.size() > 1);
    sort(names.begin(), names.end());
    for (int i = 0; i < names.size(); i++) {
      std::cout << names[i] << "\n";
    }
  } else if (command == "write-tree") {
    std::string tree_hash = write_tree(".");
    if (tree_hash.empty()) {
      std::cerr << "Error in writing tree object\n";
      return EXIT_FAILURE;
    }
    std::cout << tree_hash << "\n";
  } else if (command == "commit-tree") {
    if (argc < 5) {
      std::cerr << "Less than 5 argument " << '\n';
      return EXIT_FAILURE;
    }
    string treeSha = argv[2];
    string parentSha = "";
    string commitMsg = "defult";
    string flagg = argv[3];
    if (flagg == "-p") {
      parentSha = argv[4];
      string flagg2 = argv[5];
      if (flagg2 == "-m") {
        commitMsg = argv[6];
      } else {
        std::cerr << "Pls provide m " << '\n';
        return EXIT_FAILURE;
      }
    } else if (flagg == "-m") {
      commitMsg = argv[4];
    } else {
      std::cerr << "No such flag " << '\n';
      return EXIT_FAILURE;
    }
    commit_tree(treeSha, parentSha, commitMsg);
  } else if (command == "clone") {
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
