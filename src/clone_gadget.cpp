#include "../include/clone_gadget.h"

// Function implementations
void compressFile(const std::string data, uLong *bound, unsigned char *dest) {
  compress(dest, bound, (const Bytef *)data.c_str(), data.size());
}

std::string sha_file(std::string data) {
  unsigned char hash[20];
  SHA1((unsigned char *)data.c_str(), data.size(), hash);
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (const auto &byte : hash) {
    ss << std::setw(2) << static_cast<int>(byte);
  }
  return ss.str();
}

std::string hash_object(std::string file) {
  // Read file contents
  std::ifstream t(file);
  std::stringstream data;
  data << t.rdbuf();
  // Create blob content string
  std::string content =
      "blob " + std::to_string(data.str().length()) + '\0' + data.str();
  // Calculate SHA1 hash
  std::string buffer = sha_file(content);
  // Compress blob content
  uLong bound = compressBound(content.size());
  unsigned char compressedData[bound];
  compressFile(content, &bound, compressedData);
  // Write compressed data to .git/objects
  std::string dir = ".git/objects/" + buffer.substr(0, 2);
  std::filesystem::create_directory(dir);
  std::string objectPath = dir + "/" + buffer.substr(2);
  std::ofstream objectFile(objectPath, std::ios::binary);
  objectFile.write((char *)compressedData, bound);
  objectFile.close();
  return buffer;
}

std::string write_tree(const std::string dir_path) {
  namespace fs = std::filesystem;
  std::vector<std::pair<std::string, std::string>> entries;
  std::string mode;
  std::string sha1;
  for (const auto &entry : fs::directory_iterator(dir_path)) {
    std::string name = entry.path().filename().string();
    if (name == ".git")
      continue;
    if (entry.is_directory()) {
      mode = "40000";
      sha1 = write_tree(entry.path().string());
    } else if (entry.is_regular_file()) {
      mode = "100644";
      sha1 = hash_object(entry.path().string());
    }
    if (!sha1.empty()) {
      std::string binary_sha;
      for (size_t i = 0; i < sha1.length(); i += 2) {
        std::string byte_string = sha1.substr(i, 2);
        char byte = static_cast<char>(std::stoi(byte_string, nullptr, 16));
        binary_sha.push_back(byte);
      }
      std::string entry_data = mode + " " + name + '\0' + binary_sha;
      entries.push_back({name, entry_data});
    }
  }
  sort(entries.begin(), entries.end());
  std::string tree_content;
  for (auto &it : entries) {
    tree_content += it.second;
  }
  std::string tree_store =
      "tree " + std::to_string(tree_content.size()) + '\0' + tree_content;
  unsigned char hash[20];
  SHA1((unsigned char *)tree_store.c_str(), tree_store.size(), hash);
  std::string tree_sha;
  for (int i = 0; i < 20; i++) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(2)
       << static_cast<int>(hash[i]);
    tree_sha += ss.str();
  }
  std::string tree_dir = ".git/objects/" + tree_sha.substr(0, 2);
  std::filesystem::create_directory(tree_dir);
  std::string tree_filepath = tree_dir + "/" + tree_sha.substr(2);
  z_stream zs;
  memset(&zs, 0, sizeof(zs));
  if (deflateInit(&zs, Z_BEST_COMPRESSION) != Z_OK) {
    throw(std::runtime_error("deflateInit failed while compressing."));
  }
  zs.next_in = (Bytef *)tree_store.c_str();
  zs.avail_in = tree_store.size();
  int ret;
  char outBuffer[32768];
  std::string outstring;
  do {
    zs.next_out = reinterpret_cast<Bytef *>(outBuffer);
    zs.avail_out = sizeof(outBuffer);
    ret = deflate(&zs, Z_FINISH);
    if (outstring.size() < zs.total_out) {
      outstring.insert(outstring.end(), outBuffer,
                       outBuffer + zs.total_out - outstring.size());
    }
  } while (ret == Z_OK);
  deflateEnd(&zs);
  if (ret != Z_STREAM_END) {
    throw(std::runtime_error("Exception during zlib compression: " +
                             std::to_string(ret)));
  }
  std::ofstream outfile(tree_filepath, std::ios::binary);
  outfile.write(outstring.c_str(), outstring.size());
  outfile.close();
  return tree_sha;
}

std::pair<std::string, std::string>
object_path_from_sha(const std::string &sha) {
  std::string folder_name = sha.substr(0, 2);
  std::string file_name = sha.substr(2);
  std::string folderpath = ".git/objects/" + folder_name;
  std::string filepath = ".git/objects/" + folder_name + "/" + file_name;
  return std::make_pair(folderpath, filepath);
}

void commit_tree(std::string treeSha, std::string parSha, std::string comMsg) {
  std::ostringstream commit_body;
  commit_body << "tree " << treeSha << '\n';
  commit_body << "parent " << parSha << '\n';
  commit_body << "author XXX YYY <xxx.yyy@gmail.com> 1620000000 +0000\n";
  commit_body << "committer XXX YYY <xxx.yyy@gmail.com> 1620000000 +0000\n";
  commit_body << '\n' << comMsg << '\n';
  std::string commit = "commit " + std::to_string(commit_body.str().size()) +
                       '\x00' + commit_body.str();
  std::string tree_sha = sha_file(commit);
  std::cout << tree_sha;
  auto [folderpath, filepath] = object_path_from_sha(tree_sha);
  std::filesystem::create_directories(folderpath);
  uLong bound = compressBound(commit.size());
  unsigned char compressedData[bound];
  compressFile(commit, &bound, compressedData);
  std::ofstream output_file(filepath, std::ios::binary);
  output_file.write((char *)compressedData, bound);
  output_file.close();
}

bool git_init(const std::string &dir) {
  std::cout << "git init \n";
  try {
    std::filesystem::create_directory(dir + "/.git");
    std::filesystem::create_directory(dir + "/.git/objects");
    std::filesystem::create_directory(dir + "/.git/refs");
    std::ofstream headFile(dir + "/.git/HEAD");
    if (headFile.is_open()) {
      headFile << "ref: refs/heads/master\n";
      headFile.close();
    } else {
      std::cerr << "Failed to create .git/HEAD file.\n";
      return false;
    }

    std::cout << "Initialized git directory in " << dir << "\n";
    return true;
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << e.what() << '\n';
    return false;
  }
}

/**
 * @brief 计算字符串的SHA-1哈希值
 * @param data 待计算哈希的输入字符串
 * @param print_out 是否将哈希结果输出到控制台（可选参数，默认为false）
 * @return 返回40个字符的十六进制SHA-1哈希字符串
 */
std::string compute_sha1(const std::string &data, bool print_out) {
  // 创建20字节的缓冲区用于存储SHA-1哈希结果
  unsigned char hash[20];

  // 使用OpenSSL的SHA1函数计算输入数据的哈希值
  SHA1(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(),
       hash);

  // 创建字符串流用于构建十六进制哈希字符串
  std::stringstream ss;

  // 设置字符串流格式：十六进制、用0填充
  ss << std::hex << std::setfill('0');

  // 遍历哈希结果的每个字节，转换为十六进制格式
  for (const auto &byte : hash) {
    ss << std::setw(2) << static_cast<int>(byte);
  }

  // 如果设置了print_out参数，将哈希结果输出到控制台
  if (print_out) {
    std::cout << ss.str() << std::endl;
  }

  // 返回构建好的十六进制哈希字符串
  return ss.str();
}

int compress(FILE *input, FILE *output) {
  z_stream stream = {0};
  if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
    std::cerr << "Failed to initialize compression stream.\n";
    return EXIT_FAILURE;
  }
  char in[CHUNK];
  char out[CHUNK];
  int ret;
  int flush;
  do {
    stream.avail_in = fread(in, 1, CHUNK, input);
    stream.next_in = reinterpret_cast<unsigned char *>(in);
    if (ferror(input)) {
      (void)deflateEnd(&stream);
      std::cerr << "Failed to read from input file.\n";
      return EXIT_FAILURE;
    }
    flush = feof(input) ? Z_FINISH : Z_NO_FLUSH;
    do {
      stream.avail_out = CHUNK;
      stream.next_out = reinterpret_cast<unsigned char *>(out);
      ret = deflate(&stream, flush);
      if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
        (void)deflateEnd(&stream);
        std::cerr << "Failed to compress file.\n";
        return EXIT_FAILURE;
      }
      size_t have = CHUNK - stream.avail_out;
      if (fwrite(out, 1, have, output) != have || ferror(output)) {
        (void)deflateEnd(&stream);
        std::cerr << "Failed to write to output file.\n";
        return EXIT_FAILURE;
      }
    } while (stream.avail_out == 0);
  } while (flush != Z_FINISH);
  if (deflateEnd(&stream) != Z_OK) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

/**
 * @brief 压缩并存储Git对象到本地仓库
 * @param hash Git对象的SHA-1哈希值（40字符十六进制字符串）
 * @param content 待压缩和存储的对象内容
 * @param dir 目标目录路径（可选参数，默认为当前目录）
 */
void compress_and_store(const std::string &hash, const std::string &content,
                        std::string dir) {
  // 将内容字符串转换为内存文件流，用于压缩处理
  FILE *input = fmemopen((void *)content.c_str(), content.length(), "rb");

  // 从哈希值中提取前2个字符作为文件夹名（Git对象存储的目录结构）
  std::string hash_folder = hash.substr(0, 2);

  // 构建完整的对象存储路径：dir/.git/objects/hash_folder/
  std::string object_path = dir + "/.git/objects/" + hash_folder + '/';

  // 检查对象目录是否存在，如果不存在则创建
  if (!std::filesystem::exists(object_path)) {
    std::filesystem::create_directories(object_path);
  }

  // 构建完整的对象文件路径：dir/.git/objects/hash_folder/hash_remaining
  std::string object_file_path = object_path + hash.substr(2);

  // 检查对象文件是否已存在，避免重复存储
  if (!std::filesystem::exists(object_file_path)) {
    // 以二进制写入模式打开对象文件
    FILE *output = fopen(object_file_path.c_str(), "wb");

    // 调用压缩函数将输入内容压缩到输出文件
    if (compress(input, output) != EXIT_SUCCESS) {
      std::cerr << "Failed to compress data.\n";
      return;
    }

    // 关闭输出文件流
    fclose(output);
  }

  // 关闭输入文件流
  fclose(input);
}

/**
 * @brief 解析Git pack文件中的变长编码长度字段
 * @param pack 包含pack文件数据的字符串
 * @param pos 当前解析位置的指针，函数会更新此位置
 * @return 解析得到的长度值
 * @note 实现Git pack协议的变长编码解析，最高位为1表示继续读取下个字节
 */
int read_length(const std::string &pack, int *pos) {
  int length = 0;
  // 提取当前字节的低4位作为长度的初始值
  length |= pack[*pos] & 0x0F;

  // 检查最高位是否为1，如果是则需要继续读取后续字节
  if (pack[*pos] & 0x80) {
    (*pos)++; // 移动到下一个字节

    // 循环读取直到遇到最高位为0的字节
    while (pack[*pos] & 0x80) {
      length <<= 7;                // 左移7位为新的数据腾出空间
      length |= pack[*pos] & 0x7F; // 提取7位有效数据并合并
      (*pos)++;                    // 继续移动到下一个字节
    }

    // 处理最后一个字节（最高位为0）
    length <<= 7;         // 最后一次左移
    length |= pack[*pos]; // 提取完整的8位数据
  }

  (*pos)++; // 移动到下一个要解析的位置
  return length;
}

/**
 * @brief 应用Git delta压缩数据到基础对象，重建完整对象内容
 * @param delta_contents delta压缩指令数据
 * @param base_contents 基础对象的完整内容
 * @return 重建后的完整对象内容字符串
 * @note 实现Git pack协议中的delta解压缩算法，支持复制和添加两种指令
 */
std::string apply_delta(const std::string &delta_contents,
                        const std::string &base_contents) {
  std::string reconstructed_object;
  int current_position_in_delta = 0;

  // 跳过delta头部两个长度字段（基础对象长度和目标对象长度）
  read_length(delta_contents, &current_position_in_delta);
  read_length(delta_contents, &current_position_in_delta);

  // 逐指令处理delta数据
  while (current_position_in_delta < delta_contents.length()) {
    // 读取指令字节并后移位置
    unsigned char current_instruction =
        delta_contents[current_position_in_delta++];

    if (current_instruction & 0x80) {     // 最高位为1：复制指令
      int copy_offset = 0;                // 从基础对象的复制起始位置
      int copy_size = 0;                  // 复制的数据长度
      int bytes_processed_for_offset = 0; // 已处理的偏移量字节数

      // 解析4个字节的复制偏移量（可选，由指令字节的位3-0控制）
      for (int i = 3; i >= 0; i--) {
        copy_offset <<= 8;                    // 左移为新的字节腾出空间
        if (current_instruction & (1 << i)) { // 检查对应位是否设置
          copy_offset += static_cast<unsigned char>(
              delta_contents[current_position_in_delta + i]);
          bytes_processed_for_offset++;
        }
      }

      // 解析3个字节的复制长度（可选，由指令字节的位6-4控制）
      int bytes_processed_for_size = 0;
      for (int i = 6; i >= 4; i--) {
        copy_size <<= 8;                      // 左移为新的字节腾出空间
        if (current_instruction & (1 << i)) { // 检查对应位是否设置
          // 注意：长度参数紧跟在偏移量参数之后
          copy_size += static_cast<unsigned char>(
              delta_contents[current_position_in_delta + i -
                             (4 - bytes_processed_for_offset)]);
          bytes_processed_for_size++;
        }
      }

      // 特殊处理：如果复制长度为0，则默认为1MB
      if (copy_size == 0) {
        copy_size = 0x100000;
      }

      // 从基础对象复制指定范围的数据到重建对象
      reconstructed_object += base_contents.substr(copy_offset, copy_size);

      // 前移位置跳过已处理的参数数据
      current_position_in_delta +=
          bytes_processed_for_size + bytes_processed_for_offset;

    } else { // 最高位为0：添加指令
      // 直接从delta数据中添加新内容
      int add_size = current_instruction & 0x7F; // 提取低7位作为添加长度
      reconstructed_object +=
          delta_contents.substr(current_position_in_delta, add_size);
      current_position_in_delta += add_size; // 前移位置
    }
  }
  return reconstructed_object;
}

/**
 * @brief 将二进制SHA-1摘要转换为40字符的十六进制哈希字符串
 * @param digest 20字节的二进制SHA-1摘要数据
 * @return 返回40个字符的十六进制SHA-1哈希字符串，格式化为小写
 * @note 这是Git对象存储中的标准哈希格式，用于对象标识和文件路径构建
 */
std::string digest_to_hash(const std::string &digest) {
  std::stringstream ss;
  for (unsigned char c : digest) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
  }
  return ss.str();
}

/**
 * @brief 使用zlib库解压缩字符串
 * @param compressed_str 待解压缩的字符串数据
 * @return 解压缩后的原始字符串
 * @throws std::runtime_error 如果解压缩过程中发生错误
 */
std::string decompress_string(const std::string &compressed_str) {
  // 初始化zlib解压缩流结构体
  z_stream d_stream;

  // 将解压缩流结构体清零，确保所有字段初始化为0
  memset(&d_stream, 0, sizeof(d_stream));

  // 初始化解压缩流，如果失败则抛出异常
  if (inflateInit(&d_stream) != Z_OK) {
    throw(std::runtime_error("inflateInit failed while decompressing."));
  }

  // 设置输入缓冲区指向压缩字符串的数据
  d_stream.next_in =
      reinterpret_cast<Bytef *>(const_cast<char *>(compressed_str.data()));

  // 设置输入数据的可用大小
  d_stream.avail_in = compressed_str.size();

  // 存储解压缩操作的状态码
  int status;

  // 定义解压缩缓冲区大小（32KB）
  const size_t buffer_size = 32768;

  // 创建解压缩缓冲区
  char buffer[buffer_size];

  // 存储最终解压缩结果的字符串
  std::string decompressed_str;

  // 循环执行解压缩操作，直到所有数据被处理完毕
  do {
    // 设置输出缓冲区指向临时缓冲区
    d_stream.next_out = reinterpret_cast<Bytef *>(buffer);

    // 设置输出缓冲区的可用大小
    d_stream.avail_out = buffer_size;

    // 执行解压缩操作，0表示无特殊标志
    status = inflate(&d_stream, 0);

    // 如果解压缩数据量增加，将新数据追加到结果字符串
    if (decompressed_str.size() < d_stream.total_out) {
      decompressed_str.append(buffer, d_stream.total_out - decompressed_str.size());
    }
  } while (status == Z_OK); // 继续循环直到解压缩完成或出错

  // 结束解压缩流，如果失败则抛出异常
  if (inflateEnd(&d_stream) != Z_OK) {
    throw(std::runtime_error("inflateEnd failed while decompressing."));
  }

  // 检查解压缩是否正常结束，如果不是则抛出详细错误信息
  if (status != Z_STREAM_END) {
    std::ostringstream oss;
    oss << "Exception during zlib decompression: (" << status << ") "
        << d_stream.msg;
    throw(std::runtime_error(oss.str()));
  }

  // 返回解压缩后的字符串
  return decompressed_str;
}

/**
 * @brief 使用zlib库压缩字符串数据
 * @param input_str 待压缩的输入字符串
 * @return 压缩后的字符串数据
 * @throws std::runtime_error 如果压缩过程中发生错误
 * @note 使用DEFLATE算法进行压缩，压缩级别为Z_DEFAULT_COMPRESSION
 */
std::string compress_string(const std::string &input_str) {
  // 初始化zlib压缩流结构体
  z_stream c_stream;
  memset(&c_stream, 0, sizeof(c_stream));

  // 初始化压缩流，使用默认压缩级别，如果失败则抛出异常
  if (deflateInit(&c_stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
    throw(std::runtime_error("deflateInit failed while compressing."));
  }

  // 设置输入缓冲区指向输入字符串数据
  c_stream.next_in =
      reinterpret_cast<Bytef *>(const_cast<char *>(input_str.data()));
  c_stream.avail_in = input_str.size();

  int status;                       // 存储压缩操作的状态码
  const size_t buffer_size = 32768; // 定义输出缓冲区大小（32KB）
  char buffer[buffer_size];         // 创建输出缓冲区
  std::string compressed_str;       // 存储压缩结果的字符串

  // 循环执行压缩操作，直到所有数据被处理完毕
  do {
    // 设置输出缓冲区指针和可用大小
    c_stream.next_out = reinterpret_cast<Bytef *>(buffer);
    c_stream.avail_out = sizeof(buffer);

    // 执行压缩操作，Z_FINISH表示这是最后一块数据
    status = deflate(&c_stream, Z_FINISH);

    // 如果压缩数据量增加，将新数据追加到结果字符串
    if (compressed_str.size() < c_stream.total_out) {
      compressed_str.append(buffer, c_stream.total_out - compressed_str.size());
    }
  } while (status == Z_OK); // 继续循环直到压缩完成

  // 结束压缩流，如果失败则抛出异常
  if (deflateEnd(&c_stream) != Z_OK) {
    throw(std::runtime_error("deflateEnd failed while compressing."));
  }

  // 检查压缩是否正常结束，如果不是则抛出详细错误信息
  if (status != Z_STREAM_END) {
    std::ostringstream oss;
    oss << "Exception during zlib compression: (" << status << ") "
        << c_stream.msg;
    throw(std::runtime_error(oss.str()));
  }

  return compressed_str; // 返回压缩后的字符串
}

size_t write_callback(void *received_data, size_t element_size,
                      size_t num_element, void *userdata) {
  size_t total_size = element_size * num_element;
  std::string received_text((char *)received_data, num_element);
  std::string *master_hash = (std::string *)userdata;
  if (received_text.find("servie=git-upload-pack") == std::string::npos) {
    size_t hash_pos = received_text.find("refs/heads/master\n");
    if (hash_pos != std::string::npos) {
      *master_hash = received_text.substr(hash_pos - 41, 40);
    }
  }
  return total_size;
}

size_t pack_data_callback(void *received_data, size_t element_size,
                          size_t num_element, void *userdata) {
  std::string *accumulated_data = (std::string *)userdata;
  *accumulated_data += std::string((char *)received_data, num_element);
  return element_size * num_element;
}

/**
 * @brief 通过HTTP协议与Git远程仓库通信，获取pack文件和分支信息
 * @param url 远程Git仓库的URL地址
 * @return 返回包含pack文件数据和master分支哈希的pair对
 * @note 实现Git智能HTTP协议，先获取info/refs再下载pack文件
 */
std::pair<std::string, std::string> curl_request(const std::string &url) {
  // 初始化libcurl句柄
  CURL *handle = curl_easy_init();
  if (handle) {
    // 第一步：获取远程仓库的info/refs信息，包含分支和对象引用
    curl_easy_setopt(handle, CURLOPT_URL,
                     (url + "/info/refs?service=git-upload-pack").c_str());

    // 设置回调函数处理响应数据，提取master分支的哈希值
    std::string packhash;
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&packhash);
    curl_easy_perform(handle); // 执行HTTP请求

    // 重置curl句柄，准备下一个请求
    curl_easy_reset(handle);

    // 第二步：通过git-upload-pack服务下载pack文件数据
    curl_easy_setopt(handle, CURLOPT_URL, (url + "/git-upload-pack").c_str());

    // 构建Git协议请求数据：want指定需要的对象，done表示请求结束
    std::string postdata = "0032want " + packhash + "\n" + "00000009done\n";
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postdata.c_str());

    // 设置pack文件数据的接收回调
    std::string pack;
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&pack);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, pack_data_callback);

    // 设置HTTP请求头，指定Git上传包请求的内容类型
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(
        headers, "Content-Type: application/x-git-upload-pack-request");
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

    // 执行pack文件下载请求
    curl_easy_perform(handle);

    // 清理资源：释放curl句柄和HTTP头链表
    curl_easy_cleanup(handle);
    curl_slist_free_all(headers);

    // 返回pack文件数据和master分支哈希
    return {pack, packhash};
  } else {
    std::cerr << "Failed to initialize curl.\n";
    return {};
  }
}

int decompress(FILE *input, FILE *output) {
  // initialize decompression stream
  z_stream stream = {0};
  if (inflateInit(&stream) != Z_OK) {
    std::cerr << "Failed to initialize decompression stream.\n";
    return EXIT_FAILURE;
  }
  // initialize decompression variables
  char in[CHUNK];
  char out[CHUNK];
  bool haveHeader = false;
  char header[64];
  int ret;
  do {
    stream.avail_in = fread(in, 1, CHUNK, input);
    stream.next_in = reinterpret_cast<unsigned char *>(in);
    if (ferror(input)) {
      std::cerr << "Failed to read from input file.\n";
      return EXIT_FAILURE;
    }
    if (stream.avail_in == 0) {
      break;
    }
    do {
      stream.avail_out = CHUNK;
      stream.next_out = reinterpret_cast<unsigned char *>(out);
      ret = inflate(&stream, Z_NO_FLUSH);
      if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
        std::cerr << "Failed to decompress file.\n";
        return EXIT_FAILURE;
      }
      // write header to output file
      unsigned headerLen = 0, dataLen = 0;
      if (!haveHeader) {
        sscanf(out, "%s %u", header, &dataLen);
        haveHeader = true;
        headerLen = strlen(out) + 1;
      }
      // write decompressed data to output file
      if (dataLen > 0) {
        if (fwrite(out + headerLen, 1, dataLen, output) != dataLen) {
          std::cerr << "Failed to write to output file.\n";
          return EXIT_FAILURE;
        }
      }
    } while (stream.avail_out == 0);

  } while (ret != Z_STREAM_END);
  return inflateEnd(&stream) == Z_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

int cat_file_for_clone(const char *file_path, const std::string &dir,
                       FILE *dest, bool print_out = false) {
  try {
    std::string blob_sha = file_path;
    std::string blob_path = dir + "/.git/objects/" + blob_sha.insert(2, "/");
    if (print_out)
      std::cout << "blob path: " << blob_path << std::endl;
    FILE *blob_file = fopen(blob_path.c_str(), "rb");
    if (blob_file == NULL) {
      std::cerr << "Invalid object hash.\n";
      return EXIT_FAILURE;
    }
    decompress(blob_file, dest);
    fclose(blob_file);
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

/**
 * @brief 从Git tree对象中递归恢复文件和目录结构
 * @param tree_hash tree对象的SHA-1哈希值（40字符十六进制字符串）
 * @param dir 目标恢复目录路径
 * @param proj_dir Git项目根目录路径（包含.git目录）
 * @note 该函数递归处理tree对象，创建对应的目录结构并恢复文件内容
 */
void restore_tree(const std::string &tree_hash, const std::string &dir,
                  const std::string &proj_dir) {
  // 构建Git对象文件路径：proj_dir/.git/objects/xx/xxxxxx...（前2字符作为目录，剩余38字符作为文件名）
  std::string object_path = proj_dir + "/.git/objects/" +
                            tree_hash.substr(0, 2) + '/' + tree_hash.substr(2);

  // 以二进制模式打开tree对象文件
  std::ifstream master_tree(object_path);

  // 创建字符串流用于读取文件全部内容
  std::ostringstream buffer;
  buffer << master_tree.rdbuf();

  // 使用zlib解压缩tree对象内容（Git对象都是压缩存储的）
  std::string tree_contents = decompress_string(buffer.str());

  // 跳过tree对象头信息（格式为"tree <size>\0"），只保留实际的条目数据
  tree_contents = tree_contents.substr(tree_contents.find('\0') + 1);

  // 遍历tree对象中的每个条目（文件或子目录）
  int pos = 0;
  while (pos < tree_contents.length()) {
    // 检查当前条目是否为目录（模式为"40000"表示目录）
    if (tree_contents.find("40000", pos) == pos) {
      pos += 6; // 跳过模式字符串"40000"和后面的空格（共6个字符）

      // 提取目录路径名（从当前位置到下一个空字符之间的内容）
      std::string path =
          tree_contents.substr(pos, tree_contents.find('\0', pos) - pos);
      pos += path.length() + 1; // 跳过路径字符串和结束空字符

      // 提取子tree对象的20字节二进制SHA-1哈希值，并转换为40字符十六进制字符串
      std::string next_hash = digest_to_hash(tree_contents.substr(pos, 20));

      // 在目标目录中创建子目录
      std::filesystem::create_directory(dir + '/' + path);

      // 递归调用restore_tree处理子目录，恢复其中的文件和子目录
      restore_tree(next_hash, dir + '/' + path, proj_dir);

      pos += 20; // 跳过20字节的哈希值，继续处理下一个条目
    } else {
      // 处理文件条目（模式为"100644"表示普通文件）
      pos += 7; // 跳过模式字符串"100644"和后面的空格（共7个字符）

      // 提取文件名（从当前位置到下一个空字符之间的内容）
      std::string path =
          tree_contents.substr(pos, tree_contents.find('\0', pos) - pos);
      pos += path.length() + 1; // 跳过文件名和结束空字符

      // 提取blob对象的20字节二进制SHA-1哈希值，并转换为40字符十六进制字符串
      std::string blob_hash = digest_to_hash(tree_contents.substr(pos, 20));

      // 以二进制写入模式创建新文件
      FILE *new_file = fopen((dir + '/' + path).c_str(), "wb");

      // 调用cat_file_for_clone函数从Git对象库中读取blob内容并写入文件
      cat_file_for_clone(blob_hash.c_str(), proj_dir, new_file, false);

      // 关闭文件句柄
      fclose(new_file);

      pos += 20; // 跳过20字节的哈希值，继续处理下一个条目
    }
  }
}

/**
 * @brief Git克隆功能的主函数，实现从远程仓库克隆到本地目录
 * @param url 远程Git仓库的URL地址
 * @param dir 本地目标目录路径
 * @return 执行成功返回EXIT_SUCCESS，失败返回EXIT_FAILURE
 */
int clone(std::string url, std::string dir) {
  // 创建目标目录并初始化Git仓库
  std::filesystem::create_directory(dir);
  if (git_init(dir) != true) {
    std::cerr << "Failed to initialize git repository.\n";
    return EXIT_FAILURE;
  }

  // 通过HTTP请求获取远程仓库的pack文件和master分支哈希
  auto [pack, packhash] = curl_request(url);

  // 解析pack文件头，提取对象数量（4字节大端序）
  int num_objects = 0;
  for (int i = 8; i < 12; i++) {
    num_objects = num_objects << 8;
    num_objects = num_objects | (unsigned char)pack[i]; // 这也算一种复制操作
  }

  // 移除HTTP响应头（前20字节）和尾部的pack文件校验和（后20字节）
  pack = pack.substr(20, pack.length() - 40);

  // 处理Git pack文件中的所有对象
  int object_type;
  int current_position = 0;
  std::string master_commit_contents;

  // 遍历pack文件中的所有Git对象
  for (int object_index = 0; object_index < num_objects; object_index++) {
    // 从对象数据的第一个字节提取对象类型（高3位）
    object_type = (pack[current_position] & 112) >> 4; // 112是二进制11100000

    // 读取对象的长度（变长编码）
    int object_length = read_length(pack, &current_position);

    // 根据对象类型进行不同的处理逻辑
    if (object_type == 6) { // 偏移量delta对象（暂不支持）
      throw std::invalid_argument("Offset deltas not implemented.\n");
    } else if (object_type == 7) { // 引用delta对象处理
      // 提取基础对象的20字节SHA-1引用(也可以被称为摘要)
      std::string digest = pack.substr(current_position, 20);
      std::string hash = digest_to_hash(digest);
      current_position += 20;

  // 从本地对象库中读取基础对象内容
  std::ifstream file(dir + "/.git/objects/" + hash.insert(2, "/"));
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string file_contents = buffer.str();
  std::string base_object_contents = decompress_string(file_contents);

  // 提取并移除基础对象的类型前缀
  std::string object_type_extracted =
      base_object_contents.substr(0, base_object_contents.find(" "));
  base_object_contents =
      base_object_contents.substr(base_object_contents.find('\0') + 1);

  // 应用delta数据到基础对象，重建完整对象
  std::string delta_contents =
      decompress_string(pack.substr(current_position));
  // 这里最难，前面的逻辑只能算给鱼刮鱼鳞之类的小菜，apply_delta()
  // 这一步才是烹饪硬菜
  std::string reconstructed_contents =
      apply_delta(delta_contents, base_object_contents);

  // 重建对象的完整格式（类型+长度+内容）
  reconstructed_contents = object_type_extracted + ' ' +
                           std::to_string(reconstructed_contents.length()) +
                           '\0' + reconstructed_contents;

  // 计算重建对象的哈希并存储到本地对象库
  std::string object_hash = sha_file(reconstructed_contents);
  compress_and_store(object_hash.c_str(), reconstructed_contents, dir);

  // 跳过已处理的delta数据
  std::string compressed_delta = compress_string(delta_contents);
  current_position += compressed_delta.length();

  // 如果当前对象是master分支的commit，保存其内容
  if (hash.compare(packhash) == 0) {
    master_commit_contents =
        reconstructed_contents.substr(reconstructed_contents.find('\0'));
        
  }
} else { // 标准Git对象处理（commit=1, tree=2, blob=其他）
  // 解压缩对象数据内容
  std::string object_contents =
      decompress_string(pack.substr(current_position));
  current_position += compress_string(object_contents).length();

  // 根据对象类型构建对象头信息
  std::string object_type_str = (object_type == 1)   ? "commit "
                                : (object_type == 2) ? "tree "
                                                     : "blob ";
  object_contents = object_type_str +
                    std::to_string(object_contents.length()) + '\0' +
                    object_contents;

  // 计算对象哈希并存储到本地对象库
  std::string object_hash = compute_sha1(object_contents, false);
  std::string compressed_object = compress_string(object_contents);
  compress_and_store(object_hash.c_str(), object_contents, dir);

  // 如果当前对象是master分支的commit，保存其内容
  if (object_hash.compare(packhash) == 0) {
    master_commit_contents =
        object_contents.substr(object_contents.find('\0'));
  }
}
  }

  // 从master commit中提取tree哈希并恢复整个文件树结构
  std::string tree_hash = master_commit_contents.substr(
      master_commit_contents.find("tree") + 5, 40);
  restore_tree(tree_hash, dir, dir);

  return EXIT_SUCCESS;
}
