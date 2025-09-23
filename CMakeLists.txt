# cmake_minimum_required(VERSION 3.13)

# project(codecrafters-git)

# set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

# file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

# find_package(OpenSSL REQUIRED)
# find_package(ZLIB REQUIRED)

# add_executable(git ${SOURCE_FILES})

# target_link_libraries(git PRIVATE OpenSSL::Crypto)
# target_link_libraries(git PRIVATE ZLIB::ZLIB)

cmake_minimum_required(VERSION 3.13)

project(codecrafters-git)

# 设置默认构建类型为Debug
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# 改为使用更稳定的C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 强制添加完整的调试信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")

# 禁用所有优化并确保调试信息
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-inline -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -fno-inline -fno-omit-frame-pointer")

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(git ${SOURCE_FILES})

# 为特定目标添加调试选项
target_compile_options(git PRIVATE -g -O0 -fno-inline)
target_link_options(git PRIVATE -g)

target_link_libraries(git PRIVATE OpenSSL::Crypto)
target_link_libraries(git PRIVATE ZLIB::ZLIB)
