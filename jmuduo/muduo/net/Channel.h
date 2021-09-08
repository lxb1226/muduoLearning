// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <muduo/base/Timestamp.h>

namespace muduo
{
namespace net
{

class EventLoop;

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
/**
 * channel类(事件处理器)负责注册与响应I/O事件，但是它不拥有文件描述符。
 * 每一个channel对象自始至终都只属于一个EventLoop。
 */ 
class Channel : boost::noncopyable
{
 public:
  typedef boost::function<void()> EventCallback;  // 事件的回调处理函数
  typedef boost::function<void(Timestamp)> ReadEventCallback;   // 读事件的回调处理函数

  // 构造函数，一个EventLoop有多个Channel，一个Channel只有一个EventLoop
  Channel(EventLoop* loop, int fd);
  ~Channel();

  // 处理事件
  void handleEvent(Timestamp receiveTime);
  // 设置读回调函数（参数是TcpConnection注册的）
  void setReadCallback(const ReadEventCallback& cb)
  { readCallback_ = cb; }
  // 设置写回调函数
  void setWriteCallback(const EventCallback& cb)
  { writeCallback_ = cb; }
  // 设置关闭回调函数
  void setCloseCallback(const EventCallback& cb)
  { closeCallback_ = cb; }
  // 设置错误处理回调函数
  void setErrorCallback(const EventCallback& cb)
  { errorCallback_ = cb; }

  /// Tie this channel to the owner object managed by shared_ptr,
  /// prevent the owner object being destroyed in handleEvent.
  // 把当前事件处理器绑定到某一个对象上
  void tie(const boost::shared_ptr<void>&);

  // 返回文件描述符
  int fd() const { return fd_; }
  // 返回该事件处理所需要的处理的事件
  int events() const { return events_; }
  // 设置实际活动的事件
  void set_revents(int revt) { revents_ = revt; } // used by pollers
  // int revents() const { return revents_; }

  // 判断是否有事件
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  // 启用读（按位或后赋值），然后更新通道中的事件
  void enableReading() { events_ |= kReadEvent; update(); }
  // 禁用读（按位与后赋值）
  // void disableReading() { events_ &= ~k  ReadEvent; update(); }
  // 启用写
  void enableWriting() { events_ |= kWriteEvent; update(); }
  // 禁用写
  void disableWriting() { events_ &= ~kWriteEvent; update(); }
  // 启用所有
  void disableAll() { events_ = kNoneEvent; update(); }
  // 是否正在写
  bool isWriting() const { return events_ & kWriteEvent; }

  // for Poller
  // 返回索引
  int index() { return index_; }
  // 设置索引
  void set_index(int idx) { index_ = idx; }

  // for debug
  // 用于调试，把事件转换为字符串
  string reventsToString() const;

  // 不记录hup事件
  void doNotLogHup() { logHup_ = false; }

  // 把所属的事件循环（一个EventLoop可以由多个channel，但是一个channel只属于一个EventLoop）
  EventLoop* ownerLoop() { return loop_; }
  // 从事件循环对象中把自己删除
  void remove();

 private:
  // 更新
  void update();
  // 处理事件
  void handleEventWithGuard(Timestamp receiveTime);

  // 事件标记
  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop* loop_;     // 所属EventLoop
  const int  fd_;       // 文件描述符，但不负责关闭该文件描述符
  int        events_;   // 关注的事件
  int        revents_;  // poll/epoll返回的事件
  int        index_; // used by Poller. 表示在poll的事件数组中的序号
  bool       logHup_;   // for POLLHUP

  boost::weak_ptr<void> tie_;
  bool tied_;
  bool eventHandling_;    // 是否处于处理事件中

  // 事件回调
  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};

}
}
#endif  // MUDUO_NET_CHANNEL_H
