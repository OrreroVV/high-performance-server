cmake_minimum_required(VERSION 3.0)

#项目名称
project(sylar)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)  #
# 设置编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

# 导入当前文件夹下所有
include_directories(.)
include_directories(/apps/sylar/include)
link_directories(/apps/sylar/lib)

find_library(YAMLCPP yaml-cpp)
message("***", ${YAMLCPP})

#添加LIB_SRC 为下列文件
set(LIB_SRC
    sylar/log.cc
    sylar/util.cc
    sylar/config.cc
    )

#把LIB_SRC添加到名为sylar的库中
add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar) # __FILE__ ,显示相对位置

# 生成可执行文件test，将sylar作为依赖项
add_executable(test "tests/test.cc" sylar "${LIBS}")
#test 依赖于sylar
add_dependencies(test sylar)
#把test加入到__FILE__宏定义中,显示相对位置
force_redefine_file_macro_for_sources(test) 
#test 链接库sylar
target_link_libraries(test sylar ${YAMLCPP})

# 生成可执行文件test，将sylar作为依赖项
add_executable(test_config "tests/test_config.cc" sylar "${LIBS}")
#test 依赖于sylar
add_dependencies(test_config sylar)
#把test加入到__FILE__宏定义中,显示相对位置
force_redefine_file_macro_for_sources(test_config) 
#test 链接库sylar
target_link_libraries(test_config sylar ${YAMLCPP})

# 设置可执行文件和库文件的输出路径
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)