#include "refs.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
namespace fs = std::filesystem;
/**
 * @brief 初始化Git引用系统
 */
void MiniGitRef::Init() {
  fs::create_directory(".git/refs/heads");
  std::ofstream main_file(".git/refs/heads/main");
  std::ofstream head_file(".git/HEAD");
  head_file << "ref: refs/heads/main\n";
}

std::string MiniGitRef::GetCurrentCommit() const {
  // 获取当前HEAD指向的提交哈希
  std::string head_content = ReadRefFile(HEAD_path_);
  if (head_content.find("ref: ") == 0) {
    std::string branch_path = head_content.substr(5); // 去掉 "ref: " 前缀
    // 去除可能的换行符
    branch_path.erase(branch_path.find_last_not_of("\n\r") + 1);
    return ReadRefFile(".git/" + branch_path);
  }
  // 如果HEAD直接是提交哈希, 一般而言不会是直接提交哈希
  return head_content;
}
/**
 * @brief 切换到指定分支
 *
 * @param name 分支名称
 */
void MiniGitRef::SwitchToBranch(const std::string &name) {
  if (!BranchExists(name)) {
    throw std::runtime_error("Branch '" + name + "' does not exist!");
  }
  std::string branch_ref = "ref: refs/heads/" + name;
  std::ofstream head_file(HEAD_path_);
  head_file << branch_ref << "\n";
  std::cout << "Now working on branch: " << name << std::endl;
}
/**
 * @brief 检查分支是否存在
 *
 * @param name
 * @return true
 * @return false
 */
bool MiniGitRef::BranchExists(const std::string &name) const {
  fs::path branch_path = ".git/refs/heads/" + name;
  return fs::exists(branch_path);
}
/**
 * @brief 列出所有分支
 *
 * @return std::vector<std::string>
 */
std::vector<std::string> MiniGitRef::ListAllBranches() const {
  std::vector<std::string> branches;
  fs::path refs_heads_path = ".git/refs/heads";
  if (!fs::exists(refs_heads_path)) {
    return branches; // 目录不存在，返回空列表
  }
  for (const auto &entry : fs::directory_iterator(refs_heads_path)) {
    if (entry.is_regular_file()) { // 只处理文件，跳过子目录
      branches.push_back(entry.path().filename().string());
    }
  }
  return branches;
}
/**
 * @brief 更改一个分支指向的commit
 *
 * @param branch_name
 * @param new_hash
 * @return true
 * @return false
 */
bool MiniGitRef::UpdateBranch(const std::string &branch_name,
                              const std::string &new_hash) {
  if (!BranchExists(branch_name)) {
    throw std::runtime_error("Branch '" + branch_name + "' does not exist!");
  }

  // 基本哈希格式验证（可选）
  if (new_hash.empty() || new_hash.length() != 40) {
    throw std::runtime_error("Invalid commit hash format");
  }

  std::ofstream branch_file(".git/refs/heads/" + branch_name);
  if (!branch_file) {
    return false; // 文件打开失败
  }

  branch_file << new_hash << "\n";
  return branch_file.good(); // 检查写入是否成功
}

void MiniGitRef::WriteRefFile(const std::string &path,
                              const std::string &content) {
  // TODO: 写入引用文件
  throw std::runtime_error("WriteRefFile() not implemented yet");
}
/**
 * @brief 给定一个 git reference 的 path, 返回该ref存储的对象hash
 * @example 输入: ".git/refs/heads/main" 输出: "a1b2c3"
 * @param path
 * @return std::string
 */
auto MiniGitRef::ReadRefFile(const std::string &path) const -> std::string {
  if (path == HEAD_path_) {
    std::ifstream head_file(HEAD_path_);
    std::string head_content;
    std::getline(head_file, head_content);
    if (head_content.starts_with("ref: ")) {
      std::string branch_path = head_content.substr(5);
      std::string full_branch_path = ".git/" + branch_path;
      std::ifstream branch_file(full_branch_path);
      std::string commit_hash;
      std::getline(branch_file, commit_hash);
      return commit_hash;
    }
    return head_content;
  }
  std::ifstream ref_file(path);
  std::string content;
  std::getline(ref_file, content);
  return content;
}