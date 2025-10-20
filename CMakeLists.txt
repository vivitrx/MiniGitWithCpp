cmake_minimum_required(VERSION 3.13)

project(git-starter-cpp)

set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

add_executable(git ${SOURCE_FILES})

# Link against zlib and OpenSSL libraries
target_link_libraries(git z ssl crypto)

# Find libcurl
find_package(CURL REQUIRED)

# Link curl with your executable target without keywords
target_link_libraries(git curl)
