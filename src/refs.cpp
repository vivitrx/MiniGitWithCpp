#include "refs.h"
#include <filesystem>
#include <fstream>
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

void MiniGitRef::SwitchToBranch(const std::string &name) {
  // TODO: 切换到指定分支
  throw std::runtime_error("SwitchToBranch() not implemented yet");
}

bool MiniGitRef::BranchExists(const std::string &name) const {
  // TODO: 检查分支是否存在
  throw std::runtime_error("BranchExists() not implemented yet");
}

std::vector<std::string> MiniGitRef::ListAllBranches() const {
  // TODO: 列出所有分支
  throw std::runtime_error("ListAllBranches() not implemented yet");
}

bool MiniGitRef::UpdateBranch(const std::string &name,
                              const std::string &new_hash) {
  // TODO: 更新分支指向新的提交哈希
  throw std::runtime_error("UpdateBranch() not implemented yet");
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
