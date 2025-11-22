[![progress-banner](https://backend.codecrafters.io/progress/git/6c4b3e63-dfe2-4e2c-a55c-e04957ecd310)](https://app.codecrafters.io/users/codecrafters-bot?r=2qF)

# C++ Git 实现

这是一个用C++实现的Git版本控制系统，参与CodeCrafters的["Build Your Own Git"挑战](https://codecrafters.io/challenges/git)。

## 项目概述

本项目实现了一个功能完整的Git版本控制系统，支持以下核心功能：

- ✅ 仓库初始化 (`git init`)
- ✅ 对象操作 (`git hash-object`, `git cat-file`)
- ✅ 树对象操作 (`git write-tree`, `git ls-tree`)
- ✅ 提交管理 (`git commit-tree`)
- ✅ 远程仓库克隆 (`git clone`)

## 技术栈

- **语言**: C++17
- **构建系统**: CMake
- **依赖管理**: vcpkg
- **核心依赖**: OpenSSL, ZLIB, libcurl

## 项目结构

```
├── CMakeLists.txt          # CMake构建配置
├── vcpkg.json             # vcpkg依赖配置
├── your_program.sh        # 本地运行脚本
├── include/               # 头文件目录
│   ├── commands.h        # Git命令处理函数声明
│   ├── debug.h           # 调试工具
│   └── git_clone.h       # Git克隆功能声明
└── src/cpp/               # 源代码目录
    ├── main.cpp          # 程序入口点和命令分发
    ├── commands.cpp      # Git命令实现
    └── git_clone.cpp     # Git克隆功能实现
```

## 构建和运行

### 前置要求

确保系统已安装以下工具：
- CMake 3.13+
- C++17兼容的编译器
- vcpkg (推荐用于依赖管理)

### 本地构建

1. **克隆仓库**:
   ```sh
   git clone <repository-url>
   cd codecrafters-git-cpp
   ```

2. **构建项目**:
   ```sh
   ./your_program.sh --build-only
   ```

   或者手动构建：
   ```sh
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
   cmake --build ./build
   ```

### 运行程序

使用提供的脚本运行：
```sh
./your_program.sh <command> [options]
```

或直接运行可执行文件：
```sh
./build/git <command> [options]
```

## 已实现功能

### 1. 仓库初始化
```sh
./your_program.sh init
```
创建`.git`目录结构，包括`objects`、`refs`目录和`HEAD`文件。

### 2. 对象操作

#### 创建Blob对象
```sh
./your_program.sh hash-object -w <file>
```
将文件内容存储为Git blob对象并返回SHA1哈希。

#### 读取对象内容
```sh
./your_program.sh cat-file -p <hash>
```
显示指定哈希的Git对象内容。

### 3. 树对象操作

#### 创建树对象
```sh
./your_program.sh write-tree
```
将当前目录结构创建为Git树对象。

#### 列出树对象内容
```sh
./your_program.sh ls-tree --name-only <tree-hash>
```
显示树对象中的文件和目录列表。

### 4. 提交管理

#### 创建提交
```sh
./your_program.sh commit-tree <tree-sha> -p <parent-sha> -m "提交信息"
```
基于树对象创建提交对象。

#### 初始提交
```sh
./your_program.sh commit-tree <tree-sha> -m "初始提交"
```

### 5. 远程仓库克隆
```sh
./your_program.sh clone <repository-url> <directory>
```
克隆远程仓库到指定目录。

## 本地测试

⚠️ **重要提醒**: 为了避免损坏当前仓库的`.git`目录，请在其他目录中测试。

### 推荐测试方式

1. **使用别名** (推荐):
   ```sh
   alias mygit=/path/to/your/repo/your_program.sh
   mkdir -p /tmp/testing && cd /tmp/testing
   mygit init
   ```

2. **直接运行**:
   ```sh
   mkdir -p /tmp/testing && cd /tmp/testing
   /path/to/your/repo/your_program.sh init
   ```

### 测试示例

```sh
# 创建测试目录
mkdir -p /tmp/git-test && cd /tmp/git-test

# 初始化仓库
/path/to/codecrafters-git-cpp/your_program.sh init

# 创建测试文件
echo "Hello, Git!" > test.txt

# 创建blob对象
/path/to/codecrafters-git-cpp/your_program.sh hash-object -w test.txt

# 创建树对象
/path/to/codecrafters-git-cpp/your_program.sh write-tree

# 创建提交
/path/to/codecrafters-git-cpp/your_program.sh commit-tree <tree-sha> -m "初始提交"
```

## 调试和开发

### 调试信息
程序包含调试输出，可通过`std::cerr`查看日志信息。

### 开发注意事项
- 使用C++17标准特性
- 遵循Git对象格式规范
- 确保内存安全和异常处理
- 保持代码风格一致

## 已知限制

- 目前仅支持基本的Git命令
- 不支持分支操作
- 不支持合并功能
- 网络协议实现较为简化

## 许可证

本项目遵循MIT许可证。
