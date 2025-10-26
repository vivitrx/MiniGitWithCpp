# MiniGitWithCpp

This is a C++ implementation of a minimal Git-like version control system.

## Overview

MiniGitWithCpp is a simplified Git implementation written in C++ that demonstrates core Git concepts including:
- Object storage (blob, tree, commit)
- Delta compression
- Pack file handling
- Basic Git operations (init, add, commit, clone)

## Features

- **Object Model**: Implements Git's object model with blob, tree, and commit objects
- **Delta Compression**: Supports Git's delta compression for efficient storage
- **Pack Files**: Handles Git pack files for network transfer optimization
- **HTTP Protocol**: Implements Git's smart HTTP protocol for cloning
- **Testing**: Comprehensive unit tests using Google Test framework

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

```bash
# Initialize a repository
./git init

# Hash a file
./git hash-object <file>

# Write tree
./git write-tree

# Commit
./git commit-tree <tree-sha> -m "commit message"

# Clone from remote
./git clone <url> <directory>
```

## Testing

```bash
cd build
ctest
```

## Requirements

- C++20 compiler
- CMake 3.13+
- zlib
- OpenSSL
- libcurl
- Google Test (auto-downloaded if not present)

## License

This project is part of the [Build your own Git](https://app.codecrafters.io/courses/git/overview) challenge on CodeCrafters.
