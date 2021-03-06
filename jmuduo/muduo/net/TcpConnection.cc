// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/TcpConnection.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Socket.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

void muduo::net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
  LOG_TRACE << conn->localAddress().toIpPort() << " -> "
            << conn->peerAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
}

void muduo::net::defaultMessageCallback(const TcpConnectionPtr&,
                                        Buffer* buf,
                                        Timestamp)
{
  buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop,
                             const string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
  : loop_(CHECK_NOTNULL(loop)),
    name_(nameArg),
    state_(kConnecting),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64*1024*1024)
{
  // 向channel对象注册可读事件
  channel_->setReadCallback(
      boost::bind(&TcpConnection::handleRead, this, _1));
  channel_->setWriteCallback(
      boost::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(
      boost::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(
      boost::bind(&TcpConnection::handleError, this));
  LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this
            << " fd=" << sockfd;
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
  LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
            << " fd=" << channel_->fd();
}

void TcpConnection::send(const void* data, size_t len)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(data, len);
    }
    else
    {
      string message(static_cast<const char*>(data), len);
      loop_->runInLoop(
          boost::bind(&TcpConnection::sendInLoop,
                      this,     // FIXME
                      message));
    }
  }
}

// 实际调用的是sendInLoop,在IO线程中去调用，保证安全性
void TcpConnection::send(const StringPiece& message)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(message);
    }
    else
    {
      loop_->runInLoop(
          boost::bind(&TcpConnection::sendInLoop,
                      this,     // FIXME
                      message.as_string()));
                    //std::forward<string>(message)));
    }
  }
}

// FIXME efficiency!!!
void TcpConnection::send(Buffer* buf)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    }
    else
    {
      loop_->runInLoop(
          boost::bind(&TcpConnection::sendInLoop,
                      this,     // FIXME
                      buf->retrieveAllAsString()));
                    //std::forward<string>(message)));
    }
  }
}


void TcpConnection::sendInLoop(const StringPiece& message)
{
  sendInLoop(message.data(), message.size());
}

// 实际的发送数据函数
void TcpConnection::sendInLoop(const void* data, size_t len)
{
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool error = false;
  if (state_ == kDisconnected)
  {
    LOG_WARN << "disconnected, give up writing";
    return;
  }
  // if no thing in output queue, try writing directly
  // 如果当前channel没有写事件发生，或者发送buffer已经清空，那么就不通过缓冲区直接发送数据
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
  {
    // 发送数据
    nwrote = sockets::write(channel_->fd(), data, len);
    if (nwrote >= 0)
    {
      // 记录下没发完的数据大小
      remaining = len - nwrote;
      // 如果发完了，回调写完成函数
      if (remaining == 0 && writeCompleteCallback_)
      {
        loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
      }
    }
    // 异常处理
    else // nwrote < 0
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK)
      {
        LOG_SYSERR << "TcpConnection::sendInLoop";
        if (errno == EPIPE) // FIXME: any others?
        {
          error = true;
        }
      }
    }
  }

  // 如果还有残留的数据没有发送完成
  assert(remaining <= len);
  if (!error && remaining > 0)
  {
    LOG_TRACE << "I am going to write more data";
    size_t oldLen = outputBuffer_.readableBytes();
    // 如果输出缓冲区的数据已经超过高水位标记，那么调用highWaterMarkCallback_
    if (oldLen + remaining >= highWaterMark_
        && oldLen < highWaterMark_
        && highWaterMarkCallback_)
    {
      loop_->queueInLoop(boost::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
    }
    // 把数据添加到输出缓冲区中
    outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
    // 监听channel的可写事件（因为还有数据未发完）
    // 当可写事件被触发，就可以继续发送了，调用你的是TcpConnection::handleWrite()
    if (!channel_->isWriting())
    {
      channel_->enableWriting();
    }
  }
}


void TcpConnection::shutdown()
{
  // FIXME: use compare and swap
  if (state_ == kConnected)
  {
    setState(kDisconnecting);
    // FIXME: shared_from_this()?
    loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
  }
}

// 主动关闭，调用TcpConnection::shutdown()
void TcpConnection::shutdownInLoop()
{
  loop_->assertInLoopThread();
  // 先判断是否发送buffer里还有东西，若有，暂时不关闭
  if (!channel_->isWriting())
  {
    // we are not writing
    socket_->shutdownWrite();
  }
}

void TcpConnection::setTcpNoDelay(bool on)
{
  socket_->setTcpNoDelay(on);
}

// TcpConnection建立连接完成
void TcpConnection::connectEstablished()
{
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);  // 正处于连接建立过程
  setState(kConnected);
  channel_->tie(shared_from_this());
  // 每个连接对应一个channel，打开描述符的可读属性
  channel_->enableReading();
  // 连接成功，回调客户注册的函数(由用户提供的函数，比如OnConnection)
  connectionCallback_(shared_from_this());
}

// TcpConnection::connectDestroyed()是TcpConnection析构前最后调用的一个函数，它通知用户连接已断开
void TcpConnection::connectDestroyed()
{
  loop_->assertInLoopThread();
  if (state_ == kConnected)
  {
    setState(kDisconnected);
    channel_->disableAll();
    // 仅仅起到记录作用而已
    connectionCallback_(shared_from_this());
  }
  // 在poller中移除channel
  channel_->remove();
}

/**
 * 当某个channel有读事件发生时，会调用TcpConnection::handleRead()函数，
 * 然后从socket里读入数据到buffer，再通过回调把这些数据返回给用户层。
 */ 
void TcpConnection::handleRead(Timestamp receiveTime)
{
  loop_->assertInLoopThread();  // 断言是否在loop线程
  int savedErrno = 0;
  // 读数据到inputBuffer_中
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
  if (n > 0)
  {
    // 用户提供的处理信息的回调函数(有用户自己提供的函数，比如OnMessage)
    messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
  }
  // 读到了0，表明客户端已经关闭了
  else if (n == 0)
  {
    handleClose();
  }
  else
  {
    errno = savedErrno;
    LOG_SYSERR << "TcpConnection::handleRead";
    handleError();
  }
}

/**
 * 当可写事件触发时，调用TcpConnection::handleWrite()
 */ 
void TcpConnection::handleWrite()
{
  loop_->assertInLoopThread();
  if (channel_->isWriting())
  {
    // 写数据
    ssize_t n = sockets::write(channel_->fd(),
                               outputBuffer_.peek(),
                               outputBuffer_.readableBytes());
    if (n > 0)
    {
      // 调整发送buffer的内部index，以便下次继续发送
      outputBuffer_.retrieve(n);
      // 如果可读的数据量为0，这里的可读是针对系统发送函数来说的，不是针对用户
      // 如果对于系统发送函数来说，可读的数据量为0，表示所有数据都被发送完毕了，即写完成了
      if (outputBuffer_.readableBytes() == 0)
      {
        // 不再关注写事件
        channel_->disableWriting();
        if (writeCompleteCallback_)
        {
          // 调用用户的写完成回调函数
          loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
        }
        // 如果当前状态是正在关闭连接，那么就调用shutdown来主动关闭连接
        if (state_ == kDisconnecting)
        {
          shutdownInLoop();
        }
      }
      else
      {
        LOG_TRACE << "I am going to write more data";
      }
    }
    else
    {
      LOG_SYSERR << "TcpConnection::handleWrite";
      // if (state_ == kDisconnecting)
      // {
      //   shutdownInLoop();
      // }
    }
  }
  else
  {
    LOG_TRACE << "Connection fd = " << channel_->fd()
              << " is down, no more writing";
  }
}

/**
 * 当对端调用shutdown()关闭连接时，本端会收到一个FIN，
 * channel的读事件被触发，但inputBuffer_.readFd()会返回0，然后调用
 * handleClose()，处理关闭事件，最后调用TcpServer::removeConnection()。
 */ 
void TcpConnection::handleClose()
{
  loop_->assertInLoopThread();    // 断言是否在loop线程
  LOG_TRACE << "fd = " << channel_->fd() << " state = " << state_;
  assert(state_ == kConnected || state_ == kDisconnecting);
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  setState(kDisconnected);
  // channel上不再关注任何事情
  channel_->disableAll();
  // 获得shared_ptr交由tcpserver处理
  TcpConnectionPtr guardThis(shared_from_this());
  // 作用是记录一些日志
  connectionCallback_(guardThis);
  // must be the last line
  // 回调TcpServer::removeConnection(),TcpConnection的生命期由TcpServer控制
  closeCallback_(guardThis);
}

// 处理出错事件
void TcpConnection::handleError()
{
  int err = sockets::getSocketError(channel_->fd());
  LOG_ERROR << "TcpConnection::handleError [" << name_
            << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}

