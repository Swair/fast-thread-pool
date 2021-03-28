#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <thread>
#include <functional>
#include "FastMsgQueue.h"
#include "FastThreadPool.h"


struct Test
{
    int a;
    std::string b;
};

void show()
{
    printf("hello, fast thread pool\n");
}

bool run = true;

void Stop(int signo) 
{
    printf("oops! stop!!!\n");
    run = false;
}

int main()
{
    signal(SIGINT, Stop);

    ThreadPool tp(2);

    while(run)
    {
        std::function<void()> f = std::bind(show);
        tp.enqueue(f);
        usleep(500*1000);
    }

    return 0;
}







