cmake_minimum_required(VERSION 3.10) # CMake 最低版本要求

project(AsyncServer CXX)            # 项目名称和使用的语言 (C++)

set(CMAKE_CXX_STANDARD 17)          # 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 强制要求使用指定的 C++ 标准
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # 生成 compile_commands.json，对 IDE 和代码分析工具友好

# --- 编译选项配置 ---
# 仅保留常用的Debug和Release配置，可以根据需要调整
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-Wall -Wextra -g) # Debug模式：开启所有警告，生成调试信息
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -DNDEBUG)     # Release模式：开启O3优化，并定义NDEBUG关闭assert等调试宏
endif()

# --- 头文件搜索路径 ---
# 添加项目自身的头文件目录。假设 .h 文件在项目根目录或者与 .cpp 文件在同一目录。
include_directories(
    ${CMAKE_SOURCE_DIR}
)

# --- 定义源文件 ---
# 列出所有要编译的 .cpp 源文件
set(SERVER_SOURCES
    CServer.cpp
    CSession.cpp
    test.cpp
    # 如果有其他源文件，在这里添加
)

# --- 添加可执行目标 ---
# 创建名为 'AsyncServer' 的可执行文件
add_executable(AsyncServer ${SERVER_SOURCES})

# --- 链接库到可执行目标 (目前无外部库) ---
# 如果你的项目需要链接任何系统库（如 pthread），可以在这里添加。
# target_link_libraries(AsyncServer PRIVATE pthread)