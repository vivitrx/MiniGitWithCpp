#include "debug.h"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <zlib.h>
std::string decompress_zlib(const std::string &compressed);
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
    std::stringstream buffer;
    buffer << file.rdbuf();
    // 读取压缩数据
    buffer << file.rdbuf();
    std::string compressed_data = buffer.str();
    file.close();
    // 使用 zlib 解压缩
    std::string decompressed_data = decompress_zlib(compressed_data);
    // 解析 Git object 头部并提取内容
    // blob 对象文件的格式如下（经过 Zlib 解压缩后）：
    // blob <size>\0<content>
    size_t null_position = decompressed_data.find('\0');
    if (null_position == std::string::npos) {
      throw std::runtime_error("invalid git object format!");
    }
    std::string blob_header = decompressed_data.substr(0, null_position);
    std::string blob_content = decompressed_data.substr(null_position + 1);
    std::cout << blob_content << std::flush;
    return 0;
  } else {
    std::cerr << "Unknown command " << command << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
std::string decompress_zlib(const std::string &compressed) {
  z_stream zs;
  std::fill(reinterpret_cast<char *>(&zs),
            reinterpret_cast<char *>(&zs) + sizeof(zs), 0);

  if (inflateInit(&zs) != Z_OK) {
    throw std::runtime_error("zlib initialization failed");
  }

  zs.next_in = (Bytef *)compressed.data();
  zs.avail_in = compressed.size();

  int ret;
  char outbuffer[32768]; // 32KB的缓冲区
  std::string decompressed;

  do {
    zs.next_out = reinterpret_cast<Bytef *>(outbuffer);
    zs.avail_out = sizeof(outbuffer);

    ret = inflate(&zs, 0);

    if (decompressed.size() < zs.total_out) {
      decompressed.append(outbuffer, zs.total_out - decompressed.size());
    }
  } while (ret == Z_OK);

  inflateEnd(&zs);

  if (ret != Z_STREAM_END) {
    throw std::runtime_error("zlib decompression failed");
  }

  return decompressed;
}