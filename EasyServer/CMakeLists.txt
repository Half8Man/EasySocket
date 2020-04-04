cmake_minimum_required(VERSION 3.10)
project(EasyServer)

set(CMAKE_CXX_STANDARD 11)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(CMAKE_CXX_FLAGS "-pthread")
endif(CMAKE_SYSTEM_NAME MATCHES "Windows")

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_SRCS 变量
aux_source_directory(. DIR_SRCS)

# 指定生成目标
add_executable(EasyServer ${DIR_SRCS})