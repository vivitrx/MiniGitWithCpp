#include "commands.h"

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
  // - 第一个参数：将输入数据的指针转换为无符号字符指针(把输入数据解释成字节数组)
  // - 第二个参数：输入数据的长度（字节数）
  // - 第三个参数：输出缓冲区，用于存储计算得到的哈希值
  auto input_data = reinterpret_cast<const unsigned char *>(data.data());
  SHA1(input_data, data.size(), hash);
  // 现在你已经根据输入数据计算出一个哈希值了，哈希值就存储在 hash 里，不过你还需要把他转换成人类可读的字符串
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

std::string GetCompressedDataFromPath(const std::string &path){
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
    return compressed_data;
}