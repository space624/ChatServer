cmake_minimum_required(VERSION 3.0)
project(main)

#配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

#匹配头文件搜索路径
#include_directories()

#配置库文件搜索路径
#link_directories()

#设置需要编译的源文件的列表
set(SRC_LIST ./mudoserver.cpp)

#可执行文件的所生成的文件夹
set(EXECUTABLE_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH}/bin)


#吧.指定路径下的所有源文件名字放入变量名SRC_LIST里面
#aux_source_directory(. SRC_LIST)


add_executable(server ${SRC_LIST})

#表示server这个目标程序,需要俩姐muduo_net muduo_base pthread这三个库
target_link_libraries(server muduo_net muduo_base pthread)