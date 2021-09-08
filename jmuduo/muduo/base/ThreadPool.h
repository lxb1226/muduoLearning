// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Types.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <deque>

namespace muduo
{

class ThreadPool : boost::noncopyable
{
 public:
  typedef boost::function<void ()> Task;

  // 构造函数 显式构造
  explicit ThreadPool(const string& name = string());
  // 析构函数
  ~ThreadPool();

  // 启动线程池
  void start(int numThreads);
  // 关闭线程池
  void stop();

  // 运行任务
  void run(const Task& f);

 private:
  void runInThread();
  // 获取任务
  Task take();

  // 互斥锁
  MutexLock mutex_;
  // 条件变量
  Condition cond_;
  // 名称
  string name_;
  // 线程队列
  boost::ptr_vector<muduo::Thread> threads_;
  // 任务队列
  std::deque<Task> queue_;
  // 表示线程池是否处于运行的状态
  bool running_;
};

}

#endif
