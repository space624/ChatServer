cmake_minimum_required(VERSION 3.0)
project(Chat)

#配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

#配置最终的可执行文件的输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

#头文件搜索路径
include_directories(${PROJECT_BINARY_DIR}/include)
include_directories(${PROJECT_BINARY_DIR}/include/server)
include_directories(${PROJECT_BINARY_DIR}/include/server/db)
include_directories(${PROJECT_BINARY_DIR}/include/server/model)
include_directories(${PROJECT_BINARY_DIR}/include/server/redis)
include_directories(${PROJECT_BINARY_DIR}/thirdparty)

#源码路径
add_subdirectory(src)