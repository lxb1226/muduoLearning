// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_BLOCKINGQUEUE_H
#define MUDUO_BASE_BLOCKINGQUEUE_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>
#include <deque>
#include <assert.h>

namespace muduo
{

template<typename T>
class BlockingQueue : boost::noncopyable
{
 public:
  BlockingQueue()
    : mutex_(),
      notEmpty_(mutex_),
      queue_()
  {
  }

  // 向缓冲区中添加任务
  void put(const T& x)
  {
    MutexLockGuard lock(mutex_);
    queue_.push_back(x);
    notEmpty_.notify(); // TODO: move outside of lock
  }

  // 向缓冲区取任务
  T take()
  {
    MutexLockGuard lock(mutex_);
    // always use a while-loop, due to spurious wakeup
    while (queue_.empty())  // 避免虚假唤醒
    {
      notEmpty_.wait();   // 等待缓冲区不为空的条件（缓冲区如果空则无法取任务）
    }
    assert(!queue_.empty());
    T front(queue_.front());  // 取队头元素
    queue_.pop_front();
    return front;
  }

  size_t size() const
  {
    MutexLockGuard lock(mutex_);
    return queue_.size();
  }

 private:
  mutable MutexLock mutex_;
  Condition         notEmpty_;
  std::deque<T>     queue_;
};

}

#endif  // MUDUO_BASE_BLOCKINGQUEUE_H
