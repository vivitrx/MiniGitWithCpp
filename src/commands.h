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
#include <stdexcept>
#include <string>
#include <zlib.h>
#include <fstream>    // 如果使用文件流
#include <vector>
#include <filesystem> // 如果使用文件系统操作
#include <iostream>
std::string decompress_zlib(const std::string &compressed);
std::string compute_sha1(const std::string &data);
std::string compress_zlib(const std::string &data);
std::string GetCompressedDataFromPath(const std::string &data);
std::vector<std::string> ParseTreeObjectEntries(const std::string& entries_data);
std::string GenerateBlobObjectForFile(std::string file_name);
int HandleLsTreeCommand(int argc, char* argv[]);
std::string GetGitObjectType(const std::string& decompressed_data);
std::string GetBlobContentFromInputHash(std::string hash);