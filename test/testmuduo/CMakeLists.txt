
#配置头文件搜索路径
# include_directories()
#配置库文件搜索路径
# link_directories()

#设置需要编译的源文件列表
set(SRC_LIST ./testmuduo.cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# LIBRARY_OUTPUT_PATH

# aux_source_directory(. SRC_LIST) # 定义变量，存储当前目录下的所有源文件

add_executable(server ${SRC_LIST})

target_link_libraries(server muduo_net muduo_base pthread)

