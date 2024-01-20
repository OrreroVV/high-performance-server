cmake_minimum_required(VERSION 3.0)

#项目名称
project(sylar)

set(CMAKE_VERBOSE_MAKEFILE ON)  #
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

#添加LIB_SRC 为下列文件
set(LIB_SRC
    sylar/log.cc
    )

#把LIB_SRC添加到名为sylar的库中
add_library(sylar SHARED ${LIB_SRC})

#生成可执行文件test
add_executable(test "tests/test.cc" sylar "${LIBS}")
#test 依赖于sylar
add_dependencies(test sylar)
#test 链接库sylar
target_link_libraries(test sylar)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)