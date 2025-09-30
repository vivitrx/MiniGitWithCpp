#include "commands.h"
#include <algorithm>
#include <iostream>
#include <ostream>
#include <set>
#include <string>
namespace fs = std::filesystem;
/**
 * @brief 使用zlib解压缩数据
 * @param compressed 待解压的字符串
 * @return 解压后的原始数据
 * @throws std::runtime_error 如果解压失败
 */
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

/**
 * @brief 计算字符串的SHA-1哈希值
 * @param data 输入数据
 * @return 40字符的十六进制哈希字符串
 */
// 函数定义：接受一个常量字符串引用作为参数，返回SHA-1哈希字符串
std::string compute_sha1(const std::string &data) {
  // 声明一个无符号字符数组，用于存储SHA-1哈希结果
  // SHA_DIGEST_LENGTH = 20（SHA-1产生160位=20字节哈希）
  unsigned char hash[SHA_DIGEST_LENGTH];
  // 调用OpenSSL的SHA1函数计算哈希：
  // -
  // 第一个参数：将输入数据的指针转换为无符号字符指针(把输入数据解释成字节数组)
  // - 第二个参数：输入数据的长度（字节数）
  // - 第三个参数：输出缓冲区，用于存储计算得到的哈希值
  auto input_data = reinterpret_cast<const unsigned char *>(data.data());
  SHA1(input_data, data.size(), hash);
  // 现在你已经根据输入数据计算出一个哈希值了，哈希值就存储在 hash
  // 里，不过你还需要把他转换成人类可读的字符串
  // 创建一个字符串流对象，用于构建十六进制格式的哈希字符串
  std::stringstream ss;
  // 循环遍历哈希结果的每个字节（共20个字节）, 最终把哈希结果转化成 string 类型
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
    // 将每个字节转换为两位十六进制表示：
    // - std::hex：设置输出为十六进制格式
    // - std::setw(2)：设置输出宽度为2个字符
    // - std::setfill('0')：用前导零填充不足两位的字节
    // - static_cast<int>(hash[i])：将无符号字符转换为整数
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(hash[i]);
  }
  // 返回构建好的40字符十六进制哈希字符串
  return ss.str();
}

/**
 * @brief 使用zlib压缩数据
 * @param data 待压缩的原始数据
 * @return 压缩后的数据
 * @throws std::runtime_error 如果压缩失败
 */
// 函数定义：接受常量字符串引用作为输入，返回zlib压缩后的字符串
std::string compress_zlib(const std::string &data) {
  // 声明zlib流结构，用于管理压缩过程的状态
  z_stream zs;
  // 安全地将zs结构体的所有内存清零
  // reinterpret_cast将zs地址转换为char指针进行逐字节操作
  std::fill(reinterpret_cast<char *>(&zs),
            reinterpret_cast<char *>(&zs) + sizeof(zs), 0);
  // 初始化zlib压缩器，使用默认压缩级别
  // deflateInit()设置zs结构体的默认值并准备压缩
  // 如果返回值不等于Z_OK，表示初始化失败
  if (deflateInit(&zs, Z_DEFAULT_COMPRESSION) != Z_OK) {
    throw std::runtime_error("deflateInit failed");
  }

  // 设置输入数据：指向待压缩数据的指针
  // (Bytef *) 强制转换，因为zlib需要Bytef类型（无符号字节）
  zs.next_in = (Bytef *)data.data();

  // 设置输入数据的剩余字节数（初始为整个数据的长度）
  zs.avail_in = data.size();

  // 声明返回值变量，用于存储zlib函数的返回状态
  int ret;

  // 创建32KB的输出缓冲区，用于临时存储每次压缩的数据块
  char outbuffer[32768];

  // 创建空字符串，用于存储最终的压缩结果
  std::string compressed;

  // 开始压缩循环，使用do-while确保至少执行一次
  do {
    // 设置输出缓冲区：指向我们创建的32KB临时缓冲区
    zs.next_out = reinterpret_cast<Bytef *>(outbuffer);

    // 设置输出缓冲区的可用空间大小（32KB）
    zs.avail_out = sizeof(outbuffer);

    // 执行实际的压缩操作
    // deflate()函数从 zs.next_in 读取数据，压缩到zs.next_out
    // 参数Z_FINISH表示这是最终的数据块
    ret = deflate(&zs, Z_FINISH);

    // 检查是否有新的压缩数据需要添加到结果中
    // zs.total_out是到目前为止压缩的总字节数
    // 如果结果字符串的长度小于总压缩字节数，说明有新的数据
    if (compressed.size() < zs.total_out) {
      // 将临时缓冲区中的新数据追加到结果字符串中
      // outbuffer是源数据，zs.total_out - compressed.size()是要追加的字节数
      compressed.append(outbuffer, zs.total_out - compressed.size());
    }
  } while (ret == Z_OK); // 只要返回Z_OK就继续循环（表示还有数据要处理）

  // 压缩完成，释放zlib分配的所有资源
  deflateEnd(&zs);

  // 检查最终的返回状态，如果不是Z_STREAM_END，表示压缩失败
  // Z_STREAM_END表示成功完成整个压缩流
  if (ret != Z_STREAM_END) {
    throw std::runtime_error("zlib compression failed");
  }

  // 返回完整的压缩数据
  return compressed;
}
/**
 * @brief 从指定路径读取压缩的Git对象数据
 *
 * 该函数用于读取Git对象存储中的压缩数据文件（通常位于.git/objects目录下）。
 * 文件内容为经过zlib压缩的Git对象数据。
 *
 * @param path 要读取的文件路径（格式为.git/objects/xx/xxxx...）
 * @return std::string 包含原始压缩数据的字符串
 * @throws std::runtime_error 当文件不存在或无法打开时抛出异常
 *
 * @note 返回的是原始压缩数据，需要调用zlib解压才能获取实际对象内容
 */
std::string GetCompressedDataFromPath(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("object not found: " + path);
  }
  std::stringstream buffer;
  // 读取压缩数据
  buffer << file.rdbuf();
  std::string compressed_data = buffer.str();
  file.close();
  return compressed_data;
}

/**
 * @brief 解析树对象条目数据并生成 git ls-tree 格式的输出
 *
 * 处理格式为：<mode> <name>\0<20_byte_binary_sha> 的条目序列
 * 将二进制SHA转换为十六进制，根据mode推断类型(blob/tree)
 * 生成 "mode type sha\tname" 格式的输出字符串
 *
 * @param entries_data 树对象的条目数据（去除头部后的二进制数据）
 * @return 格式化后的ls-tree输出行向量
 */
std::vector<std::string>
ParseTreeObjectEntries(const std::string &entries_data) {
  std::vector<std::string> ls_tree_result;
  size_t offset = 0;
  /*
  树对象内容的格式
  <mode> <name>\0<20_byte_sha>
  <mode> <name>\0<20_byte_sha>
  */
  while (offset < entries_data.size()) {
    // 找到 <mode>标签 结束的位置（空格）
    size_t space_pos = entries_data.find(' ', offset);
    if (space_pos == std::string::npos)
      break;

    // 提取 <mode>标签
    std::string mode = entries_data.substr(offset, space_pos - offset);
    offset = space_pos + 1;

    // 找到文件名 <name> 结束的位置（空字符）
    size_t null_pos = entries_data.find('\0', offset);
    if (null_pos == std::string::npos)
      break;

    // 提取文件名
    std::string name = entries_data.substr(offset, null_pos - offset);
    offset = null_pos + 1;

    // 检查边界
    if (offset + 20 > entries_data.size()) {
      break;
    }

    // 提取20字节的二进制SHA
    std::string binary_sha = entries_data.substr(offset, 20);
    offset += 20;

    // 将二进制SHA转换为十六进制字符串
    std::string hex_sha;
    for (unsigned char c : binary_sha) {
      char hex[3];
      snprintf(hex, sizeof(hex), "%02x", c);
      hex_sha += hex;
    }

    // 根据mode推断类型
    std::string type;
    if (mode == "100644" || mode == "100755" || mode == "120000") {
      type = "blob";
    } else if (mode == "40000") {
      type = "tree";
    } else {
      type = "unknown"; // 处理未知类型
    }

    // 构建完整的 ls-tree 输出行
    std::string ls_tree_line = mode + " " + type + " " + hex_sha + "\t" + name;

    // 保存结果
    ls_tree_result.push_back(ls_tree_line);
  }

  return ls_tree_result;
}
/**
 * @brief 为指定文件生成Git blob对象并存储到对象数据库
 *
 * 该函数实现了Git核心的blob对象创建流程，包含以下步骤：
 * 1. 读取文件原始内容
 * 2. 构建符合Git规范的blob对象（头部+内容）
 * 3. 计算SHA-1哈希作为对象ID
 * 4. 将压缩后的对象存入.git/objects
 *
 * @param file_name 要处理的文件路径
 * @return std::string 40字符的SHA-1哈希值（小写十六进制）
 * @throws std::runtime_error 当文件读取失败时抛出异常
 *
 * @note 生成的blob对象格式：
 *       "blob {content_size}\0{file_content}"
 * @warning 文件路径必须是有效可读的，且文件大小不能超过系统限制
 * @see
 * Git对象存储规范：https://git-scm.com/book/en/v2/Git-Internals-Git-Objects
 */
std::string GenerateBlobObjectForFile(std::string file_name) {
  std::ifstream file(file_name, std::ios::binary);
  auto file_begin = std::istreambuf_iterator<char>(file);
  auto file_end = std::istreambuf_iterator<char>();
  std::string file_content{file_begin, file_end};
  file.close();
  // 2. 构建 blob 对象内容（头部 + 内容）
  int file_size = file_content.size();
  std::string blob_header = "blob " + std::to_string(file_size) + '\0';
  std::string blob_content = blob_header + file_content;
  // 3. 计算 SHA-1 哈希（对整个 blob_content）
  std::string sha1_hash = compute_sha1(blob_content);
  // 4. 创建对象目录
  std::string dir_path = ".git/objects/" + sha1_hash.substr(0, 2);
  std::filesystem::create_directories(dir_path);
  // 5. 压缩并写入对象文件
  std::string compressed_data = compress_zlib(blob_content);
  std::string target_file = dir_path + "/" + sha1_hash.substr(2);
  // ​​将要以二进制模式向
  // target_file文件中写入数据，并且后续的所有写入操作都会保持二进制格式。​​
  std::ofstream out_file{target_file, std::ios::binary};
  out_file.write(compressed_data.data(), compressed_data.size());
  out_file.close();
  return sha1_hash;
}
/**
 * @brief 解析并输出Git树对象内容
 *
 * 实现`git ls-tree`命令的核心功能，支持两种输出模式：
 * 1. 完整模式：显示所有条目信息（模式、类型、哈希、文件名）
 * 2. 精简模式(--name-only)：仅显示文件名
 *
 * @param argc 参数个数（必须≥4，如：ls-tree --name-only <tree-hash>）
 * @param argv 参数数组
 * @return int 执行状态码（0成功，非0失败）
 * @throws std::runtime_error 当出现以下情况时抛出异常：
 *         - 参数不足
 *         - 对象类型不是tree
 *         - 对象文件不存在
 *
 * @note 树对象格式：
 *       "tree <content_size>\0<entries_data>"
 *       entries_data格式：
 *       "<mode> <name>\0<20-byte-hash><mode> <name>\0<20-byte-hash>..."
 * @warning 需要确保.git/objects目录结构符合Git规范
 * @see Git树对象规范：https://git-scm.com/book/en/v2/Git-Internals-Git-Objects
 */
int ls_tree(int argc, char *argv[]) {
  std::string tree_hash = std::string(argv[3]);
  auto path =
      ".git/objects/" + tree_hash.substr(0, 2) + "/" + tree_hash.substr(2);
  // 读取并解压数据
  std::string compressed_data = GetCompressedDataFromPath(path);
  std::string decompressed_data = decompress_zlib(compressed_data);
  // 解析头部: "tree <size>\0"
  std::string object_type = GetGitObjectType(decompressed_data);
  // 验证这是树对象
  if (object_type != "tree") {
    throw std::runtime_error("Not a tree object!");
  }
  // 剩余内容是条目数据
  size_t null_pos = decompressed_data.find('\0');
  std::string entries_data = decompressed_data.substr(null_pos + 1);
  // 解析并获取文件名
  std::vector<std::string> ls_tree_result =
      ParseTreeObjectEntries(entries_data);
  if (std::string(argv[2]) == "--name-only") {
    // 只输出文件名
    for (const auto &line : ls_tree_result) {
      // 找到最后一个制表符的位置
      size_t tab_pos = line.find_last_of('\t');
      if (tab_pos != std::string::npos) {
        // 提取制表符后的文件名
        std::string name = line.substr(tab_pos + 1);
        std::cout << name << std::endl;
      }
    }
  } else {
    // 输出全部内容
    for (const auto &line : ls_tree_result) {
      std::cout << line << std::endl;
    }
  }
  return 0;
}
/**
 * @brief 获取Git对象的类型
 * @param decompressed_data 解压后的Git对象数据
 * @return 对象类型字符串（"blob", "tree", "commit", "tag"）
 * @throws std::runtime_error 如果对象格式无效
 */
std::string GetGitObjectType(const std::string &decompressed_data) {
  size_t null_pos = decompressed_data.find('\0');
  if (null_pos == std::string::npos) {
    throw std::runtime_error("Invalid git object format!");
  }

  std::string header = decompressed_data.substr(0, null_pos);
  size_t space_pos = header.find(' ');
  if (space_pos == std::string::npos) {
    throw std::runtime_error("Invalid git object header format!");
  }

  return header.substr(0, space_pos);
}
/**
 * @brief 从Git对象哈希中提取blob对象内容
 *
 * 该函数通过给定的40字符SHA-1哈希值，从Git对象数据库中获取对应的blob对象，
 * 并返回其原始内容（不包含Git对象头部信息）。
 *
 * @param hash 40字符的blob对象SHA-1哈希（小写十六进制）
 * @return std::string blob对象的原始内容
 * @throws std::runtime_error 当出现以下情况时抛出异常：
 *         - 对象文件不存在或无法读取
 *         - 解压缩失败
 *         - 对象格式无效（缺少空字符分隔符）
 *         - 对象类型不是blob（未显式检查）
 *
 * @note blob对象存储格式：
 *       "blob <content_size>\0<file_content>"
 * @warning 函数不会验证对象类型是否为blob，调用方需确保输入的是合法blob哈希
 * @see
 * Git对象存储规范：https://git-scm.com/book/en/v2/Git-Internals-Git-Objects
 */
std::string GetBlobContentFromInputHash(const std::string hash) {
  auto path = ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
  std::string compressed_data = GetCompressedDataFromPath(path);
  // 使用 zlib 解压缩
  std::string decompressed_data = decompress_zlib(compressed_data);
  // 解析 Git object 头部并提取内容
  size_t null_position = decompressed_data.find('\0');
  if (null_position == std::string::npos) {
    throw std::runtime_error("invalid git object format!");
  }
  std::string blob_header = decompressed_data.substr(0, null_position);
  std::string blob_content = decompressed_data.substr(null_position + 1);
  return blob_content;
}
/**
 * @brief 为指定目录生成Git树对象并递归处理所有子目录
 *
 * 该函数实现了Git树对象的递归生成过程，模拟`git add`的行为：
 * 1. 遍历目录中的所有文件和子目录
 * 2. 递归处理子目录生成子树对象
 * 3. 为文件生成blob对象
 * 4. 按Git规范排序条目并创建最终的树对象
 *
 * @param dir_path 要处理的目录路径
 * @return std::string 40字符的树对象SHA-1哈希（小写十六进制）
 * @throws std::runtime_error 当出现以下情况时抛出异常：
 *         - 目录不存在或无法访问
 *         - 文件读取失败
 *         - 对象存储失败
 *
 * @note 树对象条目排序规则：
 *       1. 树对象（目录）在前，数据对象（文件）在后
 *       2. 同类对象按名称ASCII码排序
 * @warning 会跳过.gitignore中指定的文件和目录
 * @see Git树对象规范：https://git-scm.com/book/en/v2/Git-Internals-Git-Objects
 */
std::string GenerateTreeObjectForDirectory(const std::string &dir_path) {
  std::vector<TreeEntry> entries;
  // 遍历目录中的所有条目
  for (const auto &entry_name : GetListOfDirectoryContents(dir_path)) {
    std::string entry_path = dir_path + "/" + entry_name;
    // 跳过 .git 目录和其他需要忽略的文件
    if (ShouldIgnoreEntry(entry_name)) {
      continue;
    }
    if (std::filesystem::is_directory(entry_name)) {
      // 递归处理子目录
      std::string sub_tree_hash = GenerateTreeObjectForDirectory(entry_path);
      entries.push_back({"40000", entry_name, HexToBin(sub_tree_hash), "tree"});
    } else {
      // 处理文件
      std::string blob_hash = GenerateBlobObjectForFile(entry_path);
      std::string mode = GetFileMode(entry_path); // 100644, 100755 等
      entries.push_back({mode, entry_name, HexToBin(blob_hash), "blob"});
    }
  }
  // 按Git要求的顺序排序条目（树对象在前，按名称排序）
  SortEntries(entries);
  // 创建并写入树对象
  return CreateAndWriteTreeObject(entries);
}
/**
 * @brief 按Git要求的顺序排序条目（树对象在前，按名称排序）
 *
 * Git树对象排序规则：
 * 1. 树对象（目录）排在数据对象（文件）前面
 * 2. 同类对象按名称字典序排序
 * 3. 名称比较区分大小写
 *
 * @param entries 要排序的条目向量
 */
void SortEntries(std::vector<TreeEntry> &entries) {
  std::sort(entries.begin(), entries.end(),
            [](const TreeEntry &a, const TreeEntry &b) {
              return a.entry_name < b.entry_name; // 严格ASCII排序
            });
}

/**
 * @brief 获取文件/目录的Git模式字符串
 *
 * 根据文件类型和权限返回对应的Git模式字符串：
 * - 目录: "040000"
 * - 符号链接: "120000"
 * - 可执行文件: "100755"
 * - 普通文件: "100644"
 *
 * @param string_path 文件系统路径（字符串形式）
 * @return std::string Git模式字符串
 * @note 使用error_code避免抛出异常，对无效路径返回"100644"作为安全值
 */
std::string GetFileMode(const std::string string_path) {
  const fs::path &path = string_path;
  std::error_code ec;
  // 检查目录类型
  if (fs::is_directory(path, ec)) {
    if (!ec)
      return "040000"; // Git目录模式
  }
  // 检查符号链接
  if (fs::is_symlink(path, ec)) {
    if (!ec)
      return "120000"; // Git符号链接模式
  }
  // 获取文件状态
  auto file_status = fs::status(path, ec);
  if (ec)
    return "100644"; // 默认安全值
  // 检查可执行权限
  return (file_status.permissions() & fs::perms::owner_exec) != fs::perms::none
             ? "100755"  // 可执行文件模式
             : "100644"; // 普通文件模式
}
/**
 * @brief 检查条目是否应该被忽略
 *
 * @param entry_name 条目名称
 * @param entry_path 条目完整路径（可选，用于更复杂的忽略规则）
 * @return true 应该忽略此条目
 * @return false 不应该忽略此条目
 */
bool ShouldIgnoreEntry(const std::string &entry_name) {
  static const std::set<std::string> ignored_entries{
      ".git", ".DS_Store", "Thumbs.db", ".vscode", ".idea", "__pycache__"};
  // 忽略以 . 开头的隐藏文件
  if (entry_name[0] == '.') {
    return true;
  }
  // 检查忽略列表
  return ignored_entries.find(entry_name) != ignored_entries.end();
}
/**
 * @brief 列出指定目录下的所有文件和子目录名称
 *
 * @param path
 * @return std::vector<std::string>
 */
std::vector<std::string> GetListOfDirectoryContents(const std::string &path) {
  std::vector<std::string> entries;
  std::error_code ec;

  // 检查路径是否存在
  if (!fs::exists(path, ec)) {
    return entries; // 路径不存在，返回空列表
  }

  // 检查是否为目录
  if (!fs::is_directory(path, ec)) {
    return entries; // 不是目录，返回空列表
  }

  // 遍历目录中的所有条目（非递归）
  for (const auto &entry : fs::directory_iterator(path, ec)) {
    if (!ec) {
      entries.push_back(entry.path().filename().string());
    }
  }

  return entries;
}
/**
 * @brief 创建并写入Git树对象
 * @param entries 待处理的条目列表（需确保hash为20字节二进制）
 * @return std::string 40字符的十六进制SHA-1哈希字符串
 * @throws std::runtime_error 如果输入无效或文件操作失败
 */
std::string CreateAndWriteTreeObject(std::vector<TreeEntry> entries) {
  // 1. 构建所有条目内容
  std::string content;
  for (const auto &entry : entries) {
    // 每个条目：mode + space + name + null + hash
    if (entry.hash.size() != 20) {
      // 先打印原始数据帮助调试
      std::cerr << "DEBUG - Raw hash bytes (" << entry.hash.size() << "): ";
      for (char c : entry.hash) {
        std::cerr << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(static_cast<unsigned char>(c)) << " ";
      }
      std::cerr << "\n";
      THROW_ERROR("Hash must be 20 raw bytes, got " +
                  std::to_string(entry.hash.size()));
    }
    content += entry.mode + " " + entry.entry_name + '\0';
    content.append(entry.hash.begin(),
                   entry.hash.end()); // 确保追加20字节二进制
  }
  // 2. 构建完整对象
  std::string header = "tree " + std::to_string(content.size()) + '\0';
  std::string tree_obj = header + content;
  // 3. 计算哈希
  auto hash = compute_sha1(tree_obj);
  // 4. 创建对象目录
  std::string dir_path = ".git/objects/" + hash.substr(0, 2);
  std::filesystem::create_directories(dir_path);
  std::string obj_path =
      ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
  // 5.存储
  std::ofstream out(obj_path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("Cannot open: " + obj_path);
  }
  std::string compressed = compress_zlib(tree_obj);
  if (!out.write(compressed.data(), compressed.size())) {
    throw std::runtime_error("Write failed: " + obj_path);
  }
  return hash;
}

auto TreeEntry::PrintTreeObjectEntries() const -> void {
  std::cerr << "--------------------------------------------------\n"
            << "| Name: " << entry_name << " | Mode: " << mode << "\n"
            << "| Hash: ";
  for (char c : hash) {
    fprintf(stderr, "%02x", static_cast<unsigned char>(c)); // 仅输出到stderr
  }
  std::cerr << "\n--------------------------------------------------\n";
}

/**
 * @brief 把十六进制字符串转化成二进制字符串
 * @param hex_str 输入字符串（自动检测十六进制或二进制）
 * @return std::string 二进制字符串
 * @throws std::invalid_argument 输入非法时抛出异常
 *
 * 智能处理逻辑：
 * 1. 如果输入长度=20且所有字符<128 → 直接返回（已经是二进制）
 * 2. 如果输入长度=40且全为十六进制字符 → 转换hex→bin
 * 3. 其他情况抛出异常
 */
std::string HexToBin(const std::string &input) {
  // 空输入检查
  if (input.empty()) {
    throw std::invalid_argument("Input string is empty");
  }

  // 情况1：可能是20字节二进制数据
  if (input.size() == 20) {
    bool is_binary = true;
    for (char c : input) {
      if (static_cast<unsigned char>(c) >= 128) {
        is_binary = false;
        break;
      }
    }
    if (is_binary)
      return input; // 直接返回二进制数据
  }

  // 情况2：40字符十六进制
  if (input.size() == 40) {
    auto is_hex = [](char c) {
      c = tolower(c);
      return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    };

    if (std::all_of(input.begin(), input.end(), is_hex)) {
      std::string binary;
      binary.reserve(20);

      for (size_t i = 0; i < 40; i += 2) {
        auto char_to_val = [](char c) -> int {
          c = tolower(c);
          return (c >= '0' && c <= '9') ? c - '0' : 10 + c - 'a';
        };

        unsigned char byte =
            (char_to_val(input[i]) << 4) | char_to_val(input[i + 1]);
        binary.push_back(static_cast<char>(byte));
      }
      return binary;
    }
  }

  // 情况3：无效输入
  throw std::invalid_argument(
      "Input must be either:\n"
      "1. 20-byte binary string (all chars < 128)\n"
      "2. 40-char hex string\n"
      "Got: " +
      std::to_string(input.size()) + " bytes with first byte: 0x" +
      (input.empty() ? "null" : std::to_string(static_cast<int>(input[0]))));
}