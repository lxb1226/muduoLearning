// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include <muduo/net/Buffer.h>

#include <muduo/net/SocketsOps.h>

#include <errno.h>
#include <sys/uio.h>

using namespace muduo;
using namespace muduo::net;

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

/*
从socket读到缓冲区的方法是使用readv先读至Buffer_,
Buffer_空间如果不够会读入到栈上65536个字节大小的空间，然后以append的方式
追加入Buffer_。既考虑了避免系统调用带来开销，又不影响数据的接收。
*/
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
  // saved an ioctl()/FIONREAD call to tell how much to read
  /*
  栈额外空间，用于从套接字往出来读时，当buffer暂时不够用时暂存数据，
  待buffer重新分配足够空间后，再把数据交换给buffer
  */
  char extrabuf[65536];
  /*
  struct iovec{
    ptr_t iov_base; //iov_base指向的缓冲区存放的是readv所接收的数据或是writev将要发送的数据
    size_t iov_len; // iov_len在各种情况下分别确定了接收的最大长度以及实际写入的长度
  }
  */
  // 使用iovec分配两个连续的缓冲区
  struct iovec vec[2];
  const size_t writable = writableBytes();
  // 第一块缓冲区，指向可写空间
  vec[0].iov_base = begin()+writerIndex_;
  vec[0].iov_len = writable;
  // 第二块缓冲区，指向栈空间
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;

  /*
    通过writev函数可以将分散保存在多个（writev的第三个参数表示缓冲区的数量）buff的数据一并进行发送，
    通过readv可以由多个buff分别接受数据，适当的使用这两个函数可以减少I/O函数的调用次数
  */
  // 如果可写空间大于65536，那么把数据直接写到缓冲区即可，否则就一个额外的栈空间
  const ssize_t n = sockets::readv(fd, vec, 2);
  if (n < 0)
  {
    *savedErrno = errno;
  } 
  // 第一块缓冲区足够容纳数据
  else if (implicit_cast<size_t>(n) <= writable)
  { 
    // 向后移动writerIndex_
    writerIndex_ += n;
  }
  // 第一块缓冲区空间不够，数据被接收到第二块缓冲区extrabuf，将其append到buffer
  else
  {
    // 更新当前writerIndex_（到buffer的末尾）
    writerIndex_ = buffer_.size();
    // 将额外空间的数据写入到缓冲区中
    append(extrabuf, n - writable);
  }
  // if (n == writable + sizeof extrabuf)
  // {
  //   goto line_30;
  // }
  return n;
}

