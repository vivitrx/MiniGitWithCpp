cmake_minimum_required(VERSION 3.13)
project(codecrafters-git)

# 设置默认构建类型为Debug
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# 使用C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译器标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fno-inline -fno-omit-frame-pointer")

# 查找依赖包
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# 手动指定源文件（更清晰明确）
set(SOURCE_FILES
  src/main.cpp
  src/commands.cpp
)

set(HEADER_FILES
  src/commands.h
  src/debug.h
)

# 创建可执行文件
add_executable(git ${SOURCE_FILES} ${HEADER_FILES})

# 设置包含目录
target_include_directories(git PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 设置目标属性
target_compile_options(git PRIVATE
    -O0 -g -fno-inline -fno-optimize-sibling-calls -fno-omit-frame-pointer
)

target_link_options(git PRIVATE
  $<$<CONFIG:Debug>:-g>
)

# 链接库
target_link_libraries(git PRIVATE
  OpenSSL::Crypto
  ZLIB::ZLIB
)

# 安装目标（可选）
install(TARGETS git DESTINATION bin)
