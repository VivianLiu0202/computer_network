# 设置CMake的最小版本需求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版本
project(MyProject VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 指定编译选项
add_compile_options(-Wall)

# 添加client可执行文件
add_executable(client client.cpp)
# 链接Windows特定的库（如果你在Windows上编译）
if(WIN32)
    target_link_libraries(client wsock32 ws2_32)
endif()

# 添加server可执行文件
add_executable(server server.cpp)
# 链接Windows特定的库（如果你在Windows上编译）
if(WIN32)
    target_link_libraries(server wsock32 ws2_32)
endif()
