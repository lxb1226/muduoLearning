// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include <muduo/base/Atomic.h>
#include <muduo/base/Types.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{

class Thread : boost::noncopyable
{
 public:
  typedef boost::function<void ()> ThreadFunc;

  // 构造函数
  explicit Thread(const ThreadFunc&, const string& name = string());
  ~Thread();

  // 启动线程
  void start();
  int join(); // return pthread_join()

  bool started() const { return started_; }
  // pthread_t pthreadId() const { return pthreadId_; }
  pid_t tid() const { return tid_; }
  const string& name() const { return name_; }

  static int numCreated() { return numCreated_.get(); }

 private:
  // 线程的入口函数
  static void* startThread(void* thread);
  // 
  void runInThread();

  bool       started_;  // 线程是否已经启动
  pthread_t  pthreadId_;  
  pid_t      tid_;  // 线程的真实pid
  ThreadFunc func_; // 回调函数
  string     name_; // 线程名字

  static AtomicInt32 numCreated_; // 已经创建的线程数量 原子数
};

}
#endif
