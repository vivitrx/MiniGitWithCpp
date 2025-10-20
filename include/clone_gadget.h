#ifndef CLONE_GADGET_H
#define CLONE_GADGET_H

#include <algorithm>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>
#include <string.h>
#include <string>
#include <vector>
#include <zlib.h>

#define CHUNK 16384 // 16KB

// Function declarations
void compressFile(const std::string data, uLong *bound, unsigned char *dest);

std::string sha_file(std::string data);

std::string hash_object(std::string file);

std::string write_tree(const std::string dir_path);

std::pair<std::string, std::string>
object_path_from_sha(const std::string &sha);

void commit_tree(std::string treeSha, std::string parSha, std::string comMsg);

bool git_init(const std::string &dir);

/**
 * @brief 计算字符串的SHA-1哈希值
 * @param data 待计算哈希的输入字符串
 * @param print_out 是否将哈希结果输出到控制台（可选参数，默认为false）
 * @return 返回40个字符的十六进制SHA-1哈希字符串
 */
std::string compute_sha1(const std::string &data, bool print_out = false);

int compress(FILE *input, FILE *output);

/**
 * @brief 压缩并存储Git对象到本地仓库
 * @param hash Git对象的SHA-1哈希值（40字符十六进制字符串）
 * @param content 待压缩和存储的对象内容
 * @param dir 目标目录路径（可选参数，默认为当前目录）
 */
void compress_and_store(const std::string &hash, const std::string &content,
                        std::string dir = ".");

/**
 * @brief 解析Git pack文件中的变长编码长度字段
 * @param pack 包含pack文件数据的字符串
 * @param pos 当前解析位置的指针，函数会更新此位置
 * @return 解析得到的长度值
 * @note 实现Git pack协议的变长编码解析，最高位为1表示继续读取下个字节
 */
int read_length(const std::string &pack, int *pos);

/**
 * @brief 应用Git delta压缩数据到基础对象，重建完整对象内容
 * @param delta_contents delta压缩指令数据
 * @param base_contents 基础对象的完整内容
 * @return 重建后的完整对象内容字符串
 * @note 实现Git pack协议中的delta解压缩算法，支持复制和添加两种指令
 */
std::string apply_delta(const std::string &delta_contents,
                        const std::string &base_contents);

/**
 * @brief 将二进制SHA-1摘要转换为40字符的十六进制哈希字符串
 * @param digest 20字节的二进制SHA-1摘要数据
 * @return 返回40个字符的十六进制SHA-1哈希字符串，格式化为小写
 * @note 这是Git对象存储中的标准哈希格式，用于对象标识和文件路径构建
 */
std::string digest_to_hash(const std::string &digest);

/**
 * @brief 使用zlib库解压缩字符串
 * @param compressed_str 待解压缩的字符串数据
 * @return 解压缩后的原始字符串
 * @throws std::runtime_error 如果解压缩过程中发生错误
 */
std::string decompress_string(const std::string &compressed_str);

/**
 * @brief 使用zlib库压缩字符串数据
 * @param input_str 待压缩的输入字符串
 * @return 压缩后的字符串数据
 * @throws std::runtime_error 如果压缩过程中发生错误
 * @note 使用DEFLATE算法进行压缩，压缩级别为Z_DEFAULT_COMPRESSION
 */
std::string compress_string(const std::string &input_str);

size_t write_callback(void *received_data, size_t element_size,
                      size_t num_element, void *userdata);

size_t pack_data_callback(void *received_data, size_t element_size,
                          size_t num_element, void *userdata);

/**
 * @brief 通过HTTP协议与Git远程仓库通信，获取pack文件和分支信息
 * @param url 远程Git仓库的URL地址
 * @return 返回包含pack文件数据和master分支哈希的pair对
 * @note 实现Git智能HTTP协议，先获取info/refs再下载pack文件
 */
std::pair<std::string, std::string> curl_request(const std::string &url);

int decompress(FILE *input, FILE *output);

int cat_file_for_clone(const char *file_path, const std::string &dir,
                       FILE *dest, bool print_out);

/**
 * @brief 从Git tree对象中递归恢复文件和目录结构
 * @param tree_hash tree对象的SHA-1哈希值（40字符十六进制字符串）
 * @param dir 目标恢复目录路径
 * @param proj_dir Git项目根目录路径（包含.git目录）
 * @note 该函数递归处理tree对象，创建对应的目录结构并恢复文件内容
 */
void restore_tree(const std::string &tree_hash, const std::string &dir,
                  const std::string &proj_dir);

/**
 * @brief Git克隆功能的主函数，实现从远程仓库克隆到本地目录
 * @param url 远程Git仓库的URL地址
 * @param dir 本地目标目录路径
 * @return 执行成功返回EXIT_SUCCESS，失败返回EXIT_FAILURE
 */
int clone(std::string url, std::string dir);

#endif // CLONE_GADGET_H
