# Install script for directory: /home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/build/debug-install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/build/debug/lib/libmuduo_base.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/muduo/base" TYPE FILE FILES
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/AsyncLogging.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/Atomic.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/BlockingQueue.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/BoundedBlockingQueue.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/Condition.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/CountDownLatch.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/CurrentThread.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/Date.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/Exception.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/FileUtil.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/LogFile.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/LogStream.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/Logging.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/Mutex.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/ProcessInfo.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/Singleton.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/StringPiece.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/Thread.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/ThreadLocal.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/ThreadLocalSingleton.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/ThreadPool.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/TimeZone.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/Timestamp.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/Types.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/copyable.h"
    "/home/heyjude/workspace/projects/cpp/projects/muduoLearning/jmuduo/muduo/base/noncopyable.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/heyjude/workspace/projects/cpp/projects/muduoLearning/build/debug/muduo/base/tests/cmake_install.cmake")

endif()

