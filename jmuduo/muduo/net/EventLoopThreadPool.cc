// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/EventLoopThreadPool.h>

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;

/**
 * EventLoopThreadPool是基于muduo库中TcpServer这个类专门做的一个线程池，它的模式属于半同步版异步，线程池中每一个线程都有一个自己的eventloop,而每一个eventloop
 * 底层都是一个poll或epoll，他利用自身的poll或者epoll在没有事件的时候阻塞住，在有事件发生的时候，epoll监听到了事件就会去处理事件
 * 
 * 
 */ 
EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop)
  : baseLoop_(baseLoop),
    started_(false),
    numThreads_(0),
    next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
  // Don't delete loop, it's stack variable
}

// 启动EventLoop线程池
void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
  assert(!started_);
  baseLoop_->assertInLoopThread();

  started_ = true;

  for (int i = 0; i < numThreads_; ++i)
  {
    // 创建一个EventLoop线程
    EventLoopThread* t = new EventLoopThread(cb);
    // 放入threads_(EventLoop线程池)
    threads_.push_back(t);
    // 启动每个EventLoopThread线程进入startLoop()，并且把返回的每个EventLoop指针压入到loops_
    loops_.push_back(t->startLoop());
  }
  if (numThreads_ == 0 && cb)
  {
    // 只有一个EventLoop，在这个EventLoo进入事件循环之前，调用cb
    cb(baseLoop_);
  }
}

// 使用round-robin(轮询调度)算法，从EventLoop线程池中选择一个EventLoop.
EventLoop* EventLoopThreadPool::getNextLoop()
{
  baseLoop_->assertInLoopThread();
  EventLoop* loop = baseLoop_;
  /**
   * 轮询调度算法的原理是每一次把来自用户的请求轮流分配给内部中的服务器，从1开始，直到N(内部服务器个数)，然后重新开始循环。
   * 轮询调度算法假设所有服务器的处理性能都相同，不关心每台服务器的当前连接数和响应速度。
   * 当请求服务间隔时间变化比较大时，轮询调度算法容易导致服务器间的负载不平衡。
   */ 

  if (!loops_.empty())
  {
    // round-robin(轮询调度)，next_记录的是下一个空闲的EventLoop
    loop = loops_[next_];
    ++next_;
    if (implicit_cast<size_t>(next_) >= loops_.size())
    {
      next_ = 0;
    }
  }
  return loop;
}

