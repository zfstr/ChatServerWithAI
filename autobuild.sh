#!/bin/bash
# 进入 build 目录
cd build

# 执行 cmake 命令
cmake ..

# 执行 make 命令
make

# 清理 build 目录
rm -rf *