set(base_SRCS
  Condition.cc
  CountDownLatch.cc
  Timestamp.cc
  Exception.cc
  Logging.cc
  LogStream.cc
  LogFile.cc
  Thread.cc
  ThreadPool.cc
  ProcessInfo.cc
  FileUtil.cc
  AsyncLogging.cc
  TimeZone.cc
  Date.cc
  )

add_library(muduo_base ${base_SRCS})
target_link_libraries(muduo_base pthread rt)

install(TARGETS muduo_base DESTINATION lib)
file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/muduo/base)

if(NOT CMAKE_BUILD_NO_EXAMPLES)
  add_subdirectory(tests)
endif()

