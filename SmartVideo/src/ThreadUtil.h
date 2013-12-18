#ifndef UTIL_THREADUTIL
#define UTIL_THREADUTIL


#include "Util.h"
#include <queue>

namespace Util
{
    /// Semaphore implementation.
    /// See: http://stackoverflow.com/a/19659736/2228771
    class Semaphore
    {
    private:
        std::mutex mtx;
        std::condition_variable cv;
        uint32 count;

    public:
        Semaphore() : count(0) {}

        /// Semaphore is active while count > 0
        bool IsActive() const { return count > 0; }

        void Notify()
        {
            std::unique_lock<std::mutex> lck(mtx);
            if (IsActive())
            {
                --count;
                if (count == 0)
                {
                    cv.notify_all();
                }
            }
        }
        
        void Wait(int newCount)
        {
            assert(newCount > 0);
            std::unique_lock<std::mutex> lck(mtx);
            count = newCount;
            cv.wait(lck, [this]() { return count > 0; });
            ++count;
        }
    };


    /// Multi-threaded queue for producer/consumer implementation.
    template<typename T>
    class ThreadSafeQueue
    {
        std::mutex queueLock;
        std::queue<T> queue;
        std::condition_variable monitor;
        int maxSize;

    public:
        ThreadSafeQueue(int maxSize = 0) : maxSize(maxSize)
        {}

        int GetSize() const { return queue.size(); }
        
        /// Add object to tail (produce).
        void Push(T& obj)
        {
            {
                // lock while adding object to queue
                std::unique_lock<std::mutex> lk(queueLock);

                // wait until space is available
                while (maxSize > 0 && queue.size() >= maxSize)
                {
                    // yield timeslice
                    monitor.wait_for(lk, chrono::milliseconds(16));
                }
                queue.push(obj);
            }
        }
        
        /// Get and remove from head (consume). Waits, while queue is empty.
        T& Pop()
        {
            {
                // lock while removing object from queue
                std::unique_lock<std::mutex> lk(queueLock);

                // wait until something is available
                while (queue.empty())
                {
                    // yield timeslice
                    monitor.wait_for(lk, chrono::milliseconds(16));
                }
                T& obj = queue.front();
                queue.pop();
                return obj;
            }
        }

        /// Remove all previously produced items. 
        /// Make sure that producers are not running anymore before taking this step.
        void Clear()
        {
            {
                // lock while removing object from queue
                std::unique_lock<std::mutex> lk(queueLock);

                // see http://stackoverflow.com/a/709161/2228771
                std::queue<T> emptyQ;
                std::swap(queue, emptyQ);
            }
        }
    };
}

#endif // UTIL_THREADUTIL