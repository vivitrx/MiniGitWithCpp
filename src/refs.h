#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>
/**
 * @brief Git引用系统核心实现
 *
 * 1. HEAD 管理
 *    功能：知道当前"我在哪里"
 *    表现：.git/HEAD文件存储当前位置
 *    附加状态：ref: refs/heads/main（指向分支）
 *    我们不实现分离状态。
 *
 * 2. 分支引用存储
 *    功能：记录每个分支的最新位置
 *    表现：.git/refs/heads/目录下的文件
 *    示例：
 *    main文件内容：a1b2c3...（40字符哈希）
 *    develop文件内容：d4e5f6...
 *
 * 3. 引用解析
 *    功能：将符号引用转换为具体提交哈希
 *    流程：HEAD → "ref: refs/heads/main" → 读取 main 文件 → "a1b2c3..." →
 * 找到提交！
 *
 * 4. 引用更新
 *    功能：移动分支"指针"到新提交
 *    操作：修改分支文件的内容
 *    示例：main文件从 a1b2c3...改为 d4e5f6...
 *
 * 该实现中不包含 tag系统 的实现
 */
class MiniGitRef {
public:
  // 初始化
  void Init(); // 返回 bool 或抛异常

  // HEAD 管理
  std::string GetCurrentCommit() const; // const 修饰
  std::string GetCurrentBranchName() const;

  // 分支管理
  std::string CreateBranch(const std::string &name); // 返回初始提交哈希
  void SwitchToBranch(const std::string &name);      // 明确动词前缀
  bool BranchExists(const std::string &name) const;
  std::vector<std::string> ListAllBranches() const;

  // 引用更新
  bool UpdateCurrentBranch(const std::string &new_hash) {
    return UpdateBranch(GetCurrentBranchName(), new_hash);
  }

private:
  // 辅助函数
  bool UpdateBranch(const std::string &branch_name,
                    const std::string &new_hash);
  void WriteRefFile(const std::string &path, const std::string &content);
  auto ReadRefFile(const std::string &path) const -> std::string;
  std::filesystem::path HEAD_path_ = ".git/HEAD";
};