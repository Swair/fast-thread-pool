#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <future>
#include <functional>
#include <stdexcept>
#include "FastMsgQueue.h"

class ThreadPool {
    public:
        inline ThreadPool(size_t threads) 
            : stop_(false)
        {
            tasks_ = new FastMsgQueue<std::function<void()> >(10240);
            for(size_t i = 0;i < threads; i++)
                workers_.emplace_back([this]
                        {
                        std::unique_ptr<std::function<void()> > task;
                        while(true)
                        {
                        try
                        {
                        task = tasks_->get();
                        }
                        catch(std::string& e)
                        {
                        printf("%s\n", e.c_str());
                        break;
                        }

                        std::function<void()> f = *task.get();
                        f(); 
                        }

                        if(this->stop_)
                        return;
                        }
            );
        }

        inline void enqueue(std::function<void()>& task)
        {
            std::unique_ptr<std::function<void()> > t(new std::function<void()>());
            *t.get() = std::move(task);
            tasks_->put(t);
        }

        inline ~ThreadPool()
        {
            stop_ = true;
            tasks_->setNonblock();
            for(std::thread &worker: workers_)
                worker.join();
            delete tasks_;
        }


    private:
        std::vector< std::thread > workers_;
        FastMsgQueue<std::function<void()> >* tasks_;
        bool stop_;
};

#endif



