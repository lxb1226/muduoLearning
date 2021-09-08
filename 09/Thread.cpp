#include "Thread.h"
#include <iostream>
using namespace std;

Thread::Thread(const ThreadFunc &func) : func_(func), autoDelete_(false)
{
    // cout << "Thread ..." << endl;
}

void Thread::Start()
{
    pthread_create(&threadId_, NULL, ThreadRoutine, this);
}

void Thread::Join()
{
    pthread_join(threadId_, NULL);
}

void Thread::SetAutoDelete(bool autoDelete)
{
    autoDelete_ = autoDelete;
}

// 静态成员函数 不能调用非静态的成员函数
// 静态成员函数 不能访问成员函数
void *Thread::ThreadRoutine(void *arg)
{
    Thread *thread = static_cast<Thread *>(arg);
    thread->Run();
    if (thread->autoDelete_)
        delete thread;
    return NULL;
}

void Thread::Run()
{
    func_();
}