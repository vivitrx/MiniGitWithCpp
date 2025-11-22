#!/bin/bash

# 构建和运行 apply_delta 测试的脚本

echo "=== 构建 apply_delta 测试 ==="

# 进入项目根目录
cd "$(dirname "$0")/.."

# 创建构建目录
mkdir -p build
cd build

# 运行 CMake 配置
echo "配置 CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 构建项目
echo "构建项目..."
make test_apply_delta

# 检查构建是否成功
if [ $? -ne 0 ]; then
    echo "构建失败！"
    exit 1
fi

echo ""
echo "=== 运行 apply_delta 测试 ==="

# 运行测试
./test_apply_delta

# 检查测试结果
if [ $? -eq 0 ]; then
    echo ""
    echo "✅ 所有测试通过！"
else
    echo ""
    echo "❌ 测试失败！"
    exit 1
fi

echo ""
echo "=== 测试完成 ==="
