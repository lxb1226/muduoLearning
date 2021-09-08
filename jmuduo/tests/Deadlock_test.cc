#include<stdio.h>
#include<time.h>
#include<pthread.h>
#include<unistd.h>

// 一个多线程程序里fork造成死锁的例子

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* doit(void* arg){
    printf("pid = %d begin doit ...\n", static_cast<int>(getpid()));
    pthread_mutex_lock(&mutex);
    struct timespec ts = {2, 0};
    nanosleep(&ts, NULL);
    pthread_mutex_unlock(&mutex);
    printf("pid = %d end doit ...\n", static_cast<int>(getpid()));

    return NULL;
}

int main(){
    printf("pid = %d Entering main ...\n", static_cast<int>(getpid()));
    pthread_t tid;
    pthread_create(&tid, NULL, doit, NULL);
    struct timespec ts = {1, 0};
    nanosleep(&ts, NULL);
    if(fork() == 0){
        doit(NULL);
    }
    pthread_join(tid, NULL);
    printf("pid = %d Exiting main ...\n", static_cast<int>(getpid()));
    return 0;
}