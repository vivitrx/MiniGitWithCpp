# apply_delta 函数 Gtest 测试框架

这个目录包含了 `apply_delta` 函数的 Google Test 测试框架。

## 文件结构

```
tests/
├── test_apply_delta.cpp    # 主要的测试文件
├── run_tests.sh           # 构建和运行测试的脚本
└── README.md              # 这个文件
```

## 测试内容

测试框架包含了以下测试用例：

### 基础功能测试
- `EmptyDeltaToEmptyBase`: 空 delta 应用到空内容
- `EmptyDeltaToNonEmptyBase`: 空 delta 应用到非空内容
- `SimpleAddInstruction`: 简单的 ADD 指令测试
- `SimpleCopyInstruction`: 简单的 COPY 指令测试
- `CopyAndAddInstructions`: COPY 和 ADD 指令组合测试

### 边界条件测试
- `CopyOutOfBounds`: COPY 超出基础内容范围
- `InvalidDeltaFormat`: 无效的 delta 数据格式
- `ParameterValidation`: 参数验证测试

### 性能和压力测试
- `LargeFileDelta`: 大文件 delta 应用测试
- `MemoryManagement`: 内存管理测试

### 实际场景测试
- `RealGitDeltaScenario`: 模拟真实的 Git delta 场景

## 使用方法

### 快速开始

运行提供的脚本来自动构建和测试：

```bash
cd tests
./run_tests.sh
```

### VS Code 调试（推荐）

我们已经配置了 VS Code 的调试环境，你可以：

1. **调试所有测试**：
   - 按 `F5` 或选择 "Debug apply_delta Tests"
   - 会自动构建并启动调试器

2. **调试特定测试**：
   - 选择 "Debug Specific Test" 配置
   - 默认调试 `SimpleAddInstruction` 测试
   - 修改 `launch.json` 中的 `--gtest_filter` 参数来调试其他测试

3. **使用 GDB 控制台**：
   - 选择 "Debug with GDB Console" 配置
   - 会在外部终端中打开 GDB，方便输入命令

### 手动构建和测试

1. 创建构建目录：
```bash
mkdir -p build
cd build
```

2. 配置 CMake：
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

3. 构建测试：
```bash
make test_apply_delta
```

4. 运行测试：
```bash
./test_apply_delta
```

### 使用 CTest

你也可以使用 CMake 的测试工具：

```bash
cd build
ctest -R ApplyDeltaTest
```

### 运行特定测试

使用 Google Test 的过滤器：

```bash
# 运行单个测试
./build/test_apply_delta --gtest_filter=ApplyDeltaTest.SimpleAddInstruction

# 运行多个匹配的测试
./build/test_apply_delta --gtest_filter=ApplyDeltaTest.Simple*

# 运行所有测试并显示详细信息
./build/test_apply_delta --gtest_output=xml:test_results.xml
```

## Google Test 框架工作原理

### 测试夹具类的工作方式

```cpp
// 测试夹具类 - Google Test 会自动管理对象生命周期
class ApplyDeltaTest : public ::testing::Test {
protected:
    void SetUp() override { /* 每个测试开始前自动调用 */ }
    void TearDown() override { /* 每个测试结束后自动调用 */ }
    
    // 辅助函数 - 可以在 TEST_F 中直接调用
    std::string createSimpleDelta(...) { return ""; }
    bool verifyReconstructedContent(...) { return true; }
};

// Google Test 会自动创建对象实例：
// TEST_F(ApplyDeltaTest, TestName) 实际上会：
// 1. 创建 ApplyDeltaTest 实例
// 2. 调用 SetUp()
// 3. 执行测试代码
// 4. 调用 TearDown()
// 5. 销毁对象
```

### 为什么不需要手动创建对象？

Google Test 的 `TEST_F` 宏会自动：
- 为每个测试用例创建独立的测试夹具实例
- 自动调用 `SetUp()` 和 `TearDown()` 
- 确保测试之间的隔离性
- 管理对象的生命周期

## 测试框架特点

1. **完整的测试覆盖**: 包含基础功能、边界条件、性能和实际场景测试
2. **易于扩展**: 使用测试夹具类，方便添加新的测试用例
3. **自动构建**: 支持自动下载和配置 Google Test
4. **详细的错误报告**: Google Test 提供详细的失败信息
5. **自动对象管理**: Google Test 自动处理测试夹具的创建和销毁

## 添加新测试

要添加新的测试用例，只需在 `test_apply_delta.cpp` 文件中添加新的 `TEST_F` 宏：

```cpp
TEST_F(ApplyDeltaTest, YourNewTestName) {
    // Arrange
    std::string delta_contents = "...";
    std::string base_contents = "...";
    
    // Act
    std::string result = apply_delta(delta_contents, base_contents);
    
    // Assert
    EXPECT_EQ(result, "expected_result");
}
```

## 依赖项

- Google Test (GTest)
- CMake 3.13+
- C++20 编译器
- zlib, OpenSSL, libcurl (与主项目相同)

## 注意事项

1. 测试框架会自动处理 Google Test 的依赖，如果系统中没有安装，会从 GitHub 下载
2. 测试用例基于当前的 `apply_delta` 实现，如果函数实现有变化，可能需要调整测试用例
3. 某些测试用例可能需要根据实际的 delta 格式进行调整

## 故障排除

如果测试构建失败：

1. 确保所有依赖项已安装：`sudo apt-get install libgtest-dev cmake build-essential`
2. 检查网络连接（用于下载 Google Test）
3. 查看构建输出中的具体错误信息
4. 确保主项目的依赖项（zlib, openssl, curl）已正确安装
