// 对于 decompress_zlib
#include <stdexcept>
#include <string>
#include <zlib.h>
// 对于 compute_sha1
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>
#include <string>
// 对于 compress_zlib
#include <bitset>     // 必须包含这个头文件
#include <filesystem> // 如果使用文件系统操作
#include <fstream>    // 如果使用文件流
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unistd.h>
#include <vector>
#include <zlib.h>
// 宏
#define THROW_ERROR(msg)                                                       \
  throw std::runtime_error(std::string(__FILE__) + ":" +                       \
                           std::to_string(__LINE__) + " in " + __func__ +      \
                           "(): " + msg)
//
std::string decompress_zlib(const std::string &compressed);
std::string compute_sha1(const std::string &data);
std::string compress_zlib(const std::string &data);
std::string GetCompressedDataFromPath(const std::string &data);
std::vector<std::string>
ParseTreeObjectEntries(const std::string &entries_data);
std::string GenerateBlobObjectForFile(std::string file_name);

//
std::string GenerateTreeObjectForFile(std::string file_name);
std::string GenerateEntitiesContent(std::string file_content);
//

int ls_tree(int argc, char *argv[]);
std::string GetGitObjectType(const std::string &decompressed_data);
std::string GetBlobContentFromInputHash(std::string hash);
/**
 * @brief 用于实现 "write-tree" 命令的结构体和配套函数 以后会考虑发展成一个
 * class
 *
 */
struct TreeEntry {
  std::string mode;
  std::string entry_name;
  std::string hash;
  std::string entry_type;
  // for debug
  auto PrintTreeObjectEntries() const -> void;
};
std::string GetCurrentWorkingDirectory();
std::string GenerateTreeObjectForDirectory(const std::string &dir_path);
bool ShouldIgnoreEntry(const std::string &entry_name);
std::string GetFileMode(const std::string ep);
// 按Git要求的顺序排序条目（树对象在前，按名称排序）
void SortEntries(std::vector<TreeEntry> &entries);

std::string
CreateAndWriteTreeObject(std::vector<TreeEntry> entries); // 返回一个 SHA1 哈希

std::vector<std::string> GetListOfDirectoryContents(const std::string &path);

std::string HexToBin(const std::string &hex_str);