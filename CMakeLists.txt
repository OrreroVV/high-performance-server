cmake_minimum_required(VERSION 3.0)

#项目名称
project(sylar)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)  #
# 设置编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations  -Werror=unused-variable")

# 导入当前文件夹下所有
include_directories(.)
include_directories(/apps/sylar/include)
link_directories(/apps/sylar/lib)

find_library(YAMLCPP yaml-cpp)

#添加LIB_SRC 为下列文件
set(LIB_SRC
    sylar/config.cc
    sylar/fiber.cc
    sylar/iomanager.cc
    sylar/log.cc
    sylar/scheduler.cc
    sylar/thread.cc
    sylar/timer.cc
    sylar/util.cc
    )

#把LIB_SRC添加到名为sylar的库中
add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar) # __FILE__ ,显示相对位置

set(LIB_LIB
    sylar 
    pthread
    ${YAMLCPP}
)

message("***", ${LIB_LIB})


# 生成可执行文件test，将sylar作为依赖项
add_executable(test "tests/test.cc" sylar "${LIBS}")
#test 依赖于sylar
add_dependencies(test sylar)
#把test加入到__FILE__宏定义中,显示相对位置
force_redefine_file_macro_for_sources(test) 
#test 链接库LIB_LIB
target_link_libraries(test sylar ${LIB_LIB})

# 生成可执行文件test_config，将sylar作为依赖项
add_executable(test_config "tests/test_config.cc" sylar "${LIBS}")
#test 依赖于sylar
add_dependencies(test_config sylar)
#把test加入到__FILE__宏定义中,显示相对位置
force_redefine_file_macro_for_sources(test_config) 
#test 链接库LIB_LIB
target_link_libraries(test_config sylar ${LIB_LIB})


# 生成可执行文件test_thread，将sylar作为依赖项
add_executable(test_thread "tests/test_thread.cc" sylar "${LIBS}")
#test 依赖于sylar
add_dependencies(test_thread sylar)
#把test加入到__FILE__宏定义中,显示相对位置
force_redefine_file_macro_for_sources(test_thread) 
#test 链接库LIB_LIB
target_link_libraries(test_thread sylar ${LIB_LIB})


# 生成可执行文件test_util，将sylar作为依赖项
add_executable(test_util "tests/test_util.cc" sylar "${LIBS}")
#test 依赖于sylar
add_dependencies(test_util sylar)
#把test加入到__FILE__宏定义中,显示相对位置
force_redefine_file_macro_for_sources(test_util) 
#test 链接库LIB_LIB
target_link_libraries(test_util sylar ${LIB_LIB})


# 生成可执行文件test_fiber，将sylar作为依赖项
add_executable(test_fiber "tests/test_fiber.cc" sylar "${LIBS}")
#test 依赖于sylar
add_dependencies(test_fiber sylar)
#把test加入到__FILE__宏定义中,显示相对位置
force_redefine_file_macro_for_sources(test_fiber) 
#test 链接库LIB_LIB
target_link_libraries(test_fiber sylar ${LIB_LIB})


# 生成可执行文件test_scheduler，将sylar作为依赖项
add_executable(test_scheduler "tests/test_scheduler.cc" sylar "${LIBS}")
#test_scheduler 依赖于sylar
add_dependencies(test_scheduler sylar)
#把test_scheduler加入到__FILE__宏定义中,显示相对位置
force_redefine_file_macro_for_sources(test_scheduler) 
#test_scheduler 链接库LIB_LIB
target_link_libraries(test_scheduler sylar ${LIB_LIB})


# 生成可执行文件test_iomanager，将sylar作为依赖项
add_executable(test_iomanager "tests/test_iomanager.cc" sylar "${LIBS}")
#test_iomanager 依赖于sylar
add_dependencies(test_iomanager sylar)
#把test_iomanager 加入到__FILE__宏定义中,显示相对位置
force_redefine_file_macro_for_sources(test_iomanager) 
#test_iomanager 链接库LIB_LIB
target_link_libraries(test_iomanager sylar ${LIB_LIB})


# 设置可执行文件和库文件的输出路径
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)