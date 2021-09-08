#ifndef MUDUO_BASE_ASYNCLOGGING_H
#define MUDUO_BASE_ASYNCLOGGING_H

#include <muduo/base/BlockingQueue.h>
#include <muduo/base/BoundedBlockingQueue.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>

#include <muduo/base/LogStream.h>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace muduo
{

class AsyncLogging : boost::noncopyable
{
 public:

  AsyncLogging(const string& basename,
               size_t rollSize,
               int flushInterval = 3);

  ~AsyncLogging()
  {
    if (running_)
    {
      stop();
    }
  }

  void append(const char* logline, int len);

  void start()
  {
    // 在构造函数中latch_的值为1
    // 在线程运行之后将latch_的减为0
    running_ = true;
    thread_.start();
    // 必须等到latch_变为0才能从start函数中返回，这表明初始化已经完成
    latch_.wait();
  }

  void stop()
  {
    running_ = false;
    cond_.notify();
    thread_.join();
  }

 private:

  // declare but not define, prevent compiler-synthesized functions
  AsyncLogging(const AsyncLogging&);  // ptr_container
  void operator=(const AsyncLogging&);  // ptr_container

  void threadFunc();

  typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer;
  
  typedef boost::ptr_vector<Buffer> BufferVector;
  typedef BufferVector::auto_type BufferPtr;  // 指向buffer的指针

  const int flushInterval_; // 定期(flushInterval_秒)将缓冲区的数据写到文件中
  bool running_;        // 是否正在运行
  string basename_;     // 日志名字
  size_t rollSize_;     // 预留的日志大小
  muduo::Thread thread_;  // 执行该异步日志记录器的线程
  muduo::CountDownLatch latch_; // 倒计时计数器初始化为1，用于指示什么时候记录器才能开始正常工作
  muduo::MutexLock mutex_;    
  muduo::Condition cond_;
  BufferPtr currentBuffer_;   // 当前的缓冲区
  BufferPtr nextBuffer_;      // 下一个缓冲区
  BufferVector buffers_;      // 缓冲区队列
};

}
#endif  // MUDUO_BASE_ASYNCLOGGING_H
