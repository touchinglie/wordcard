cmake_minimum_required(VERSION 3.10)
project(wordsCard)

# 指定C/C++标准（可选）
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# 使用pkg-config查找GTK4（自动使用系统默认的gcc/g++）
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

# 编译sqlite3源码（位于当前目录的 ./sqlite3/ 子目录）
add_library(sqlite3 ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3/sqlite3.c)

# 生成可执行文件
add_executable(wordsCard main.c)

# 为可执行文件指定头文件路径和链接库
target_include_directories(wordsCard PRIVATE ${GTK4_INCLUDE_DIRS})
target_link_libraries(wordsCard ${GTK4_LIBRARIES} sqlite3)

# 传递GTK4的额外编译选项（如宏定义）
target_compile_options(wordsCard PRIVATE ${GTK4_CFLAGS_OTHER})
