#ifndef MUDUO_BASE_NONCOPYABLE_H
#define MUDUO_BASE_NONCOPYABLE_H

namespace muduo
{

class noncopyable
{
 public:
  noncopyable(const noncopyable&) = delete; // =delete 禁止使用该函数 默认构造函数
  void operator=(const noncopyable&) = delete;  // 禁止使用默认赋值函数

 protected:
  noncopyable() = default;    // 默认构造函数
  ~noncopyable() = default;   // 默认析构函数
};

}  // namespace muduo

#endif  // MUDUO_BASE_NONCOPYABLE_H
