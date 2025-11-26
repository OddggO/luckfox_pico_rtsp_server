cmake_minimum_required(VERSION 3.10)
project(MyRtspServer)

# 使用 C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件路径
include_directories(
    ${PROJECT_SOURCE_DIR}/Src/Base
    ${PROJECT_SOURCE_DIR}/Src/Live
    ${PROJECT_SOURCE_DIR}/Src/Scheduler
)

# 递归查找所有子目录中的 cpp 文件
file(GLOB_RECURSE SRC_FILES
    ${PROJECT_SOURCE_DIR}/Src/Base/*.cpp
    ${PROJECT_SOURCE_DIR}/Src/Live/*.cpp
    ${PROJECT_SOURCE_DIR}/Src/Scheduler/*.cpp
)

# 主程序
set(MAIN_FILE ${PROJECT_SOURCE_DIR}/Src/test.cpp)

# 生成可执行文件
add_executable(myapp ${MAIN_FILE} ${SRC_FILES})

# 链接多线程（pthread）
find_package(Threads REQUIRED)
target_link_libraries(myapp Threads::Threads)

