#ifndef __MSGQUEUE_H
#define __MSGQUEUE_H

#include <memory>
#include <mutex>
#include <condition_variable>

template<typename Type>
class FastMsgQueue
{
    private:
        struct Mem
        {
            std::unique_ptr<Type>* queue;
            size_t windex; 
            size_t rindex; 
            size_t maxCount;

            Mem(size_t mc)
            {
                maxCount = mc;
                queue = new std::unique_ptr<Type>[maxCount];
                windex = 0;
                rindex = 0;
            }

            ~Mem()
            {
                delete[] queue; 
                queue = nullptr;
            }

            void push(std::unique_ptr<Type>& msg)
            {
                queue[windex] = std::move(msg);
                windex = (windex + 1) % maxCount;
            }

            std::unique_ptr<Type> pop()
            {
                std::unique_ptr<Type> msg;
                msg = std::move(queue[rindex]);
                rindex = (rindex + 1) % maxCount;
                return msg;
            }
        };

    public:
        FastMsgQueue(size_t maxlen)
        {
            if(maxlen == 0)
                maxlen = 10240;
            mem_.reset(new Mem(2 * maxlen));
            msgPutCount_ = 0;
            msgGetCount_ = 0;
            msgMaxCount_ = maxlen; 
            nonblock_ = 0;
        }

        ~FastMsgQueue()
        {
        }

        void setNonblock()
        {
            nonblock_ = 1;
            std::unique_lock<std::mutex> lock(putMutex_);
            getCond_.notify_one();
            putCond_.notify_all();
        }

        void setBlock()
        {
            nonblock_ = 0;
        }

        void putData(Type& msg)
        {
            std::unique_ptr<Type> m(new Type());
            *m.get() = std::move(msg);          
            put(m);
        }

        Type getData()
        {
            std::unique_ptr<Type> m = get();
            return *m.get(); 
        }

        void put(std::unique_ptr<Type>& msg)
        {
            if(nullptr == msg)
                throw std::string("put, warning, msg is null");

            std::unique_lock<std::mutex> lock(putMutex_);   
            putCond_.wait(lock, [this]{
                    return msgPutCount_ <= msgMaxCount_ - 1 || nonblock_; });
            mem_->push(msg);
            msgPutCount_ ++;
            getCond_.notify_one();
        }

        std::unique_ptr<Type> get()
        {
            std::unique_ptr<Type> msg;
            std::unique_lock<std::mutex> lock(getMutex_);  
            if (msgGetCount_ || updateCount() > 0)
            {
                msg = std::move(mem_->pop());
                if(nullptr == msg)
                    throw std::string("warning, get null msg");

                msgGetCount_ --;
            }
            else
            {
                throw std::string("get, warning, no more data");
            }
            return msg;
        }

    private:
        size_t updateCount()
        {
            std::unique_lock<std::mutex> lock(putMutex_);
            getCond_.wait(lock, [this]{
                    return msgPutCount_ != 0 || nonblock_; });

            size_t msgCount = msgPutCount_;
            if (msgCount > msgMaxCount_ - 1)
                putCond_.notify_all();

            msgGetCount_ = msgPutCount_;   
            msgPutCount_ = 0;
            return msgCount;
        }


    private:
        std::unique_ptr<Mem> mem_;
        size_t msgPutCount_;
        size_t msgGetCount_;
        size_t msgMaxCount_;
        int nonblock_;
        std::mutex putMutex_;
        std::condition_variable putCond_; 
        std::mutex getMutex_;
        std::condition_variable getCond_; 
};


#endif



