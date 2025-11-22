cmake_minimum_required(VERSION 3.13)

project(MiniGitWithCpp)

set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

# 主项目源文件
file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

# 创建主可执行文件
add_executable(git ${SOURCE_FILES})

# Link against zlib and OpenSSL libraries
target_link_libraries(git z ssl crypto)

# Find libcurl
find_package(CURL REQUIRED)

# Link curl with your executable target without keywords
target_link_libraries(git curl)

# Google Test 配置
enable_testing()

# 查找 Google Test
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    # 如果系统中没有安装 GTest，使用 FetchContent 下载
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    FetchContent_MakeAvailable(googletest)
endif()

# 测试可执行文件
add_executable(test_apply_delta tests/test_apply_delta.cpp)

# 添加主项目源文件（排除 main 函数文件）
file(GLOB_RECURSE TEST_SOURCE_FILES 
    src/clone_gadget.cpp
    src/refs.cpp
    src/refs.h
)
target_sources(test_apply_delta PRIVATE ${TEST_SOURCE_FILES})

# 设置测试包含目录
target_include_directories(test_apply_delta PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# 链接测试所需的库
target_link_libraries(test_apply_delta 
    gtest 
    gtest_main 
    pthread
    z 
    ssl 
    crypto 
    curl
)

# 添加测试
add_test(NAME ApplyDeltaTest COMMAND test_apply_delta)

# 设置测试属性
set_tests_properties(ApplyDeltaTest PROPERTIES
    TIMEOUT 30
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
