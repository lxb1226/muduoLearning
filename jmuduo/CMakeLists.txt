cmake_minimum_required(VERSION 2.6)

project(muduo CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CXX_FLAGS
 -g
 # -DVALGRIND
 # -DMUDUO_STD_STRING
 -D_FILE_OFFSET_BITS=64 # 定义一个宏
 #-Wall  # 大部分警告
 -Wextra  # 一些额外的警告
 -Werror  # 当出现警告时转为错误，停止编译
 -Wconversion # 一些可能改变值的隐式转换，给出警告
 -Wno-unused-parameter  # 函数中出现未使用的参数，不给出警告
 -Wold-style-cast # C风格的转化，给出警告
 -Woverloaded-virtual # 如果函数的声明隐藏住了基类的虚函数，就给出警告
 -Wpointer-arith   # 对函数指针或者void * 类型的指针进行算术操作时给出警告
 -Wshadow # 当一个局部变量遮盖住了另一个局部变量，或者全局变量，给出警告
 -Wwrite-strings  # 规定字符串常量的类型是const char[length],因此把这样的地址复制给non-const char*指针将产生警告，这些警告能够帮助你在编译期间发现企图写入字符串常量的代码
 -march=native  # 指定cpu体系结构为本地平台
 # -MMD
 # -std=c++0x
 -rdynamic
 )
if(CMAKE_BUILD_BITS EQUAL 32)
  list(APPEND CXX_FLAGS "-m32")
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_COMPILER "g++")
#set(CMAKE_CXX_COMPILER "icpc")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

find_package(Boost REQUIRED)
find_package(Protobuf)
find_package(CURL)
find_path(CARES_INCLUDE_DIR ares.h)
find_library(CARES_LIBRARY NAMES cares)
find_path(MHD_INCLUDE_DIR microhttpd.h)
find_library(MHD_LIBRARY NAMES microhttpd)
find_library(BOOSTTEST_LIBRARY NAMES boost_unit_test_framework)

include_directories(${Boost_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR})

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

add_subdirectory(muduo/base)
add_subdirectory(muduo/net)
add_subdirectory(tests)

# if(NOT CMAKE_BUILD_NO_EXAMPLES)
#   add_subdirectory(examples)
# else()
#   if(CARES_INCLUDE_DIR AND CARES_LIBRARY)
#     add_subdirectory(examples/cdns)
#   endif()
# endif()

