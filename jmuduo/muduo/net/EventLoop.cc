// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/EventLoop.h>

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Singleton.h>
#include <muduo/net/Channel.h>
#include <muduo/net/Poller.h>
#include <muduo/net/SocketsOps.h>
#include <muduo/net/TimerQueue.h>

#include <boost/bind.hpp>

#include <signal.h>
#include <sys/eventfd.h>

using namespace muduo;
using namespace muduo::net;

namespace
{
__thread EventLoop* t_loopInThisThread = 0;

const int kPollTimeMs = 10000;

/**
 * eventfd是一种线程间通信机制。简单来说eventfd就是一个文件描述符，它引用了一个内核维护的eventfd object，是uint64_t类型，
 * 也就是8个字节，可以作为counter。支持read，write，以及有关epoll等操作。
 * 
 */ 
int createEventfd()
{
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0)
  {
    LOG_SYSERR << "Failed in eventfd";
    abort();
  }
  return evtfd;
}

/**
 * SIGPIPE的默认行为是终止进程，在命令行程序中这是合理的，
 * 但是在网络编程中，这意味着如果对方断开连接而本地继续写入的话，会造成
 * 服务进程意外退出。假如服务进程繁忙，没有及时处理对方断开连接的事件，
 * 就有可能出现在连接断开之后继续发送数据的情况。这样的情况是需要避免的。
 * 
 */ 
#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe
{
 public:
  IgnoreSigPipe()
  {
    ::signal(SIGPIPE, SIG_IGN);
    LOG_TRACE << "Ignore SIGPIPE";
  }
};
#pragma GCC diagnostic error "-Wold-style-cast"

IgnoreSigPipe initObj;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
  return t_loopInThisThread;
}

/**
 * 创建轮询器(Poller)，创建用于传递消息的管道，初始化各个部分，然后进入一个无限循环，每一次循环中调用
 * 轮询器的轮询函数(等待函数)，等待事件发生，如果有事件发生，就依次调用套接字(或其他的文件描述符)的事件处理器
 * 处理各个事件，然后调用投递的回调函数；接着进入下一次循环。
 * 
 */ 
EventLoop::EventLoop()
  : looping_(false),
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    iteration_(0),
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),
    timerQueue_(new TimerQueue(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_)),
    currentActiveChannel_(NULL)
{
  LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
  if (t_loopInThisThread)
  {
    LOG_FATAL << "Another EventLoop " << t_loopInThisThread
              << " exists in this thread " << threadId_;
  }
  else
  {
    t_loopInThisThread = this;
  }
  // 设置唤醒事件处理器的读回调函数为handleRead
  wakeupChannel_->setReadCallback(
      boost::bind(&EventLoop::handleRead, this));
  // we are always reading the wakeupfd
  // 启用读功能
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
  ::close(wakeupFd_);
  t_loopInThisThread = NULL;
}

/**
 * 事件循环。在某线程中实例化EventLoop对象，这个线程就是IO线程，必须在IO线程中执行loop操作，
 * 在当前IO线程中进行updateChannel，在当前线程中进行removeChannel。
 * 
 */ 
void EventLoop::loop()
{
  assert(!looping_);
  assertInLoopThread();
  looping_ = true;
  quit_ = false;
  LOG_TRACE << "EventLoop " << this << " start looping";

  while (!quit_)
  {
    activeChannels_.clear();
    // 调用poll获取活跃的channel(activeChannels_)
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    // 增加Poll次数
    ++iteration_;
    if (Logger::logLevel() <= Logger::TRACE)
    {
      printActiveChannels();
    }
    // TODO sort channel by priority
    // 事件处理开始
    eventHandling_ = true;
    // 遍历活跃的channel，执行每一个channel上的回调函数
    for (ChannelList::iterator it = activeChannels_.begin();
        it != activeChannels_.end(); ++it)
    {
      currentActiveChannel_ = *it;
      currentActiveChannel_->handleEvent(pollReturnTime_);
    }
    currentActiveChannel_ = NULL;
    // 事件处理结束
    eventHandling_ = false;
    // 处理用户在其他线程注册给IO线程的事件
    doPendingFunctors();
  }

  LOG_TRACE << "EventLoop " << this << " stop looping";
  looping_ = false;
}

// 结束EventLoop线程
void EventLoop::quit()
{
  quit_ = true;
  if (!isInLoopThread())
  {
    wakeup();
  }
}

/**
 * EventLoop线程除了等待poll、执行poll返回的激活事件，
 * 还可以处理一些其他任务，例如调用某一个回调函数，处理其他EventLoop对象的，
 * 调用EventLoop::runInLoop()即可让EventLoop线程执行cb函数。
 * 
 * 假设我们有这样的调用:loop->runInLoop(run),说明想让IO线程执行一定的计算任务，此时若是在当前的IO线程，就马上执行run()；
 * 如果是其他线程调用的，那么就执行queueInLoop(run)，将run异步添加到队列，当loop内处理完事件后，就执行doPendingFunctors()，也就执行了run();
 * 最后想要结束线程的话，执行quit。
 * 
 */ 
void EventLoop::runInLoop(const Functor& cb)
{
  // 如果是当前线程是EventLoop线程则立即执行，否则放到任务队列中，异步执行
  if (isInLoopThread())
  {
    cb();
  }
  else
  {
    queueInLoop(cb);
  }
}

// 任务队列
void EventLoop::queueInLoop(const Functor& cb)
{
  {
  MutexLockGuard lock(mutex_);
  pendingFunctors_.push_back(cb);
  }
  /**
   * 如果当前线程不是EventLoop线程，或者正在执行pendingFunctors_中的任务，
   * 都要唤醒EventLoop线程，让其执行pendingFunctors_中的任务。
   */ 
  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
}

// 在指定的事件调用TimerCallback
TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
  return timerQueue_->addTimer(cb, time, 0.0);
}

// 等一段时间之后调用TimerCallback
TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, cb);
}

// 以固定的时间反复调用TimerCallback
TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
  Timestamp time(addTime(Timestamp::now(), interval));
  return timerQueue_->addTimer(cb, time, interval);
}

// 取消Timer
void EventLoop::cancel(TimerId timerId)
{
  return timerQueue_->cancel(timerId);
}

// 更新Channel，实际上是调用poller_->updateChannel()
void EventLoop::updateChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

/**
 * EventLoop对象中poller对象也持有Channel对象的指针
 * 所有需要将Channel对象安全的从poller对象中移除
 */ 
void EventLoop::removeChannel(Channel* channel)
{
  // 每次间接的调用的作用就是将需要改动的东西与当前调用的类撇清关系
  assert(channel->ownerLoop() == this);
  // 如果没有在loop线程调用直接退出
  assertInLoopThread();
  // 判断是否在事件处理状态。判断当前是否在处理这个将要删除的事件以及活动的事件表中是否有这个事件
  if (eventHandling_)
  {
    assert(currentActiveChannel_ == channel ||
        std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
  }
  poller_->removeChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid();
}

// 使用eventfd唤醒
void EventLoop::wakeup()
{
  uint64_t one = 1;
  ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

// 执行任务队列中的任务
void EventLoop::doPendingFunctors()
{
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
  MutexLockGuard lock(mutex_);
  functors.swap(pendingFunctors_);
  }

  for (size_t i = 0; i < functors.size(); ++i)
  {
    functors[i]();
  }
  callingPendingFunctors_ = false;
}

void EventLoop::printActiveChannels() const
{
  for (ChannelList::const_iterator it = activeChannels_.begin();
      it != activeChannels_.end(); ++it)
  {
    const Channel* ch = *it;
    LOG_TRACE << "{" << ch->reventsToString() << "} ";
  }
}

