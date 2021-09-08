// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/CountDownLatch.h>

using namespace muduo;

/*
  注意声明的顺序，mutex_要先于condition_构造
*/
CountDownLatch::CountDownLatch(int count)
  : mutex_(),
    condition_(mutex_),
    count_(count)
{
}

/*
  主线程调用wait进入阻塞状态，等待计数器变为0
*/
void CountDownLatch::wait()
{
  MutexLockGuard lock(mutex_);
  while (count_ > 0) {
    condition_.wait();
  }
}

/*
  子线程调用countDown将计数器减一，计数器变为0唤醒所有线程
*/
void CountDownLatch::countDown()
{
  MutexLockGuard lock(mutex_);
  --count_;
  if (count_ == 0) {
    condition_.notifyAll();
  }
}

/*
  获取倒计时计数器的值
*/
int CountDownLatch::getCount() const
{
  MutexLockGuard lock(mutex_);
  return count_;
}

