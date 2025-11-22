// 声明这个文件属于 `internal` 包。
// `internal` 是一个特殊包名，其规则是：只有父级目录相同的包才能导入它。
// 这常用于编写不希望被外部项目引用的内部代码。
package internal

// 导入此测试代码所依赖的其他包。
import (
	"fmt"       // 格式化输入输出，用于构建错误信息
	"io/ioutil" // 用于读写文件和目录（Go 1.16+ 推荐使用 `os` 包）
	"math/rand" // 用于生成随机数，在此代码中随机选择测试仓库和测试数据
	"path"      // 提供跨操作系统的路径操作函数（如 `Join`）
	"time"      // 提供时间相关的功能，在此用于初始化随机数种子

	// 导入测试框架 "tester-utils" 的包。
	// 这个包很可能由 CodeCrafters 提供，包含用于构建阶段测试的工具函数。
	tester_utils "github.com/codecrafters-io/tester-utils"

	// 导入 go-git 库，这是一个用纯 Go 实现的 Git 客户端库。
	// 测试代码将使用这个“官方”库来验证你的自定义实现是否正确。
	"github.com/go-git/go-git/v5"
	// 导入 go-git 的 plumbing 子包。
	// "plumbing" 指的是 Git 的低层操作（对象、引用、包文件等），
	// 相对于 porcelain（高层命令，如 add, commit）。
	"github.com/go-git/go-git/v5/plumbing"
)

// 定义结构体 TestFile。
// 这是一个辅助数据结构，代表测试用的文件及其期望内容。
type TestFile struct {
	path     string // 文件在仓库中的相对路径（如 "scooby/dooby/doo"）
	contents string // 文件期望拥有的内容
}

// 定义结构体 TestRepo。
// 这是一个辅助数据结构，代表一个用于测试的远程 Git 仓库。
type TestRepo struct {
	url            string     // 远程仓库的 URL（如 "https://github.com/..."）
	exampleCommits []string   // 该仓库中可用于测试的提交哈希值列表
	exampleFiles   []TestFile // 该仓库中可用于验证的文件列表
}

// 为 TestRepo 类型定义一个方法：randomCommit。
// 它从 exampleCommits 数组中随机选择一个提交哈希并返回。
func (r TestRepo) randomCommit() string {
	return r.exampleCommits[rand.Intn(len(r.exampleCommits))]
}

// 为 TestRepo 类型定义一个方法：randomFile。
// 它从 exampleFiles 数组中随机选择一个 TestFile 并返回。
func (r TestRepo) randomFile() TestFile {
	return r.exampleFiles[rand.Intn(len(r.exampleFiles))]
}

// 初始化一个全局变量 testRepos，类型是 TestRepo 的切片（数组）。
// 这里预定义了三个测试用的远程仓库及其测试数据。
var testRepos []TestRepo = []TestRepo{
	// 第一个测试仓库
	TestRepo{
		url: "https://github.com/codecrafters-io/git-sample-1",
		exampleCommits: []string{
			"3b0466d22854e57bf9ad3ccf82008a2d3f199550", // 该仓库的一个特定提交
		},
		exampleFiles: []TestFile{
			TestFile{
				path:     "scooby/dooby/doo",                                                  // 仓库中的一个文件路径
				contents: "dooby yikes dumpty scooby monkey donkey horsey humpty vanilla doo", // 该文件的期望内容
			},
		},
	},
	// 第二个测试仓库
	TestRepo{
		url: "https://github.com/codecrafters-io/git-sample-2",
		exampleCommits: []string{
			"b521b9179412d90a893bc36f33f5dcfd987105ef",
		},
		exampleFiles: []TestFile{
			TestFile{
				path:     "humpty/vanilla/yikes",
				contents: "scooby yikes dooby",
			},
		},
	},
	// 第三个测试仓库
	TestRepo{
		url: "https://github.com/codecrafters-io/git-sample-3",
		exampleCommits: []string{
			"23f0bc3b5c7c3108e41c448f01a3db31e7064bbb",
			"b521b9179412d90a893bc36f33f5dcfd987105ef", // 这个仓库有两个测试提交
		},
		exampleFiles: []TestFile{
			TestFile{
				path:     "donkey/donkey/monkey",
				contents: "monkey humpty doo scooby dumpty donkey vanilla horsey dooby",
			},
		},
	},
}

// 函数 randomRepo 从 testRepos 数组中随机选择一个 TestRepo 并返回。
func randomRepo() TestRepo {
	rand.Seed(time.Now().UnixNano()) // 用当前时间初始化随机数种子，确保每次运行结果不同
	return testRepos[rand.Intn(3)]   // rand.Intn(3) 生成 0, 1, 2 中的随机数
}

// 这是主要的测试函数，由测试框架调用。
// stageHarness 参数提供了记录日志和执行被测程序的能力。
func testCloneRepository(stageHarness *tester_utils.StageHarness) error {
	logger := stageHarness.Logger         // 获取日志记录器，用于输出测试进度和信息
	executable := stageHarness.Executable // 获取一个执行器，用于运行你的自定义 Git 程序

	// 在系统的临时目录下创建一个临时目录。
	// 你的 Git 实现将把仓库克隆到这个目录，避免污染项目本身。
	tempDir, err := ioutil.TempDir("", "worktree")
	if err != nil { // 如果创建失败，返回错误
		return err
	}

	// 设置执行器的工作目录为刚才创建的临时目录。
	// 这意味着你的 `./your_git.sh` 命令将在 tempDir 中运行。
	executable.WorkingDir = tempDir

	// 随机选择一个测试仓库（git-sample-1, 2 或 3）
	testRepo := randomRepo()

	// 记录调试日志，显示将要执行的命令
	logger.Debugf("Running ./your_git.sh clone %s <testDir>", testRepo.url)
	// 核心操作：运行你的自定义 Git 实现。
	// 执行命令：`./your_git.sh clone <随机仓库URL> test_dir`
	// 它期望你的程序将仓库克隆到 tempDir/test_dir 目录中。
	result, err := executable.Run("clone", testRepo.url, "test_dir")
	if err != nil { // 如果执行过程本身出错（如命令不存在），返回错误
		return err
	}

	// 检查你的自定义 Git 程序的退出代码。
	// 如果退出码不是 0（成功），则测试失败。
	if err = assertExitCode(result, 0); err != nil {
		return err
	}

	// 拼接出克隆的目标目录完整路径
	repoDir := path.Join(tempDir, "test_dir")
	// 使用“官方”的 go-git 库打开刚刚由你的程序克隆下来的仓库。
	// 测试代码用权威的库来验证你的实现是否正确。
	r, err := git.PlainOpen(repoDir)
	if err != nil { // 如果 go-git 无法打开它，说明你的克隆根本失败
		return err
	}

	// 测试阶段 1：验证提交对象
	// 从测试仓库的数据中随机选择一个提交哈希
	commit_sha := testRepo.randomCommit()

	// 记录日志，显示将要检查的内容
	logger.Debugf("Running git cat-file commit %s", commit_sha)

	// 使用 go-git 库，通过完整的提交哈希获取提交对象。
	// 如果你的克隆是正确的，这个对象应该存在于本地仓库中。
	commit, err := r.CommitObject(plumbing.NewHash(commit_sha))
	if err != nil { // 如果找不到该提交对象，说明你的克隆不完整，测试失败
		return err
	}

	// 验证提交的作者名是否正确。
	// 这是一个具体的断言，确保元数据而不仅仅是数据被正确克隆。
	expected, actual := "Paul Kuruvilla", commit.Author.Name
	if expected != actual {
		// 如果不符合预期，格式化一个错误信息
		return fmt.Errorf("Expected '%s' as author name, got: '%s'", expected, actual)
	}
	// 如果成功，记录成功信息
	logger.Successf("Commit contents verified")

	// 测试阶段 2：验证文件内容
	// 从测试仓库数据中随机选择一个文件定义
	testFile := testRepo.randomFile()

	// 记录日志
	logger.Debugf("Reading contents of a sample file")
	// 使用标准库直接读取被克隆下来的文件内容
	bytes, err := ioutil.ReadFile(path.Join(repoDir, testFile.path))
	if err != nil { // 如果文件不存在，读取失败
		return err
	}

	// 比较文件的实际内容是否与预期的完全一致
	expected, actual = testFile.contents, string(bytes)
	if expected != actual {
		return fmt.Errorf("Expected '%s' as file contents, got: '%s'", expected, actual)
	}
	logger.Successf("File contents verified")

	// 所有检查都通过，返回 nil（无错误）表示测试成功
	return nil
}
