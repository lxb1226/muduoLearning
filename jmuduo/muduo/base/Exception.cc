// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/Exception.h>

//#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>

using namespace muduo;

Exception::Exception(const char* msg)
  : message_(msg)
{
  fillStackTrace();
}

Exception::Exception(const string& msg)
  : message_(msg)
{
  fillStackTrace();
}

Exception::~Exception() throw ()
{
}

const char* Exception::what() const throw()
{
  return message_.c_str();  // 返回当前异常内容指针
}

const char* Exception::stackTrace() const throw()
{
  return stack_.c_str();  // 返回所有异常内容指针
}

void Exception::fillStackTrace()
{
  const int len = 200;
  void* buffer[len];
  int nptrs = ::backtrace(buffer, len); // 系统调用，用于返回发送异常的函数和之前函数的调用总个数，存储其函数指针列表
  char** strings = ::backtrace_symbols(buffer, nptrs);  // 将函数指针列表查询返回为具体函数信息字符串
  if (strings)
  {
    for (int i = 0; i < nptrs; ++i)
    {
      // TODO demangle funcion name with abi::__cxa_demangle
      stack_.append(strings[i]);
      stack_.push_back('\n');
    }
    free(strings);
  }
}

