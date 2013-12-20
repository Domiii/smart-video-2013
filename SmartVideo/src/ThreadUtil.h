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
    public:
        typedef std::deque<T> Queue;

    private:
        std::mutex queueLock;
        Queue queue;
        std::condition_variable monitor;
        int maxSize;
        bool sorted;

        std::function<bool(const T&)> nextPredicate;
        

        /// Check if the queue contains the correct next element.
        bool ContainsValidElement()
        {
            if (!nextPredicate) return true;
            
            if (sorted) return nextPredicate(queue.front());
            else
            {
                // iterate over all elements
                for (auto& el : queue)
                    if (nextPredicate(el)) return true;

                return false;
            }
        }

    public:
        typedef std::deque<T> Queue;
        
        ThreadSafeQueue(int maxSize = 0, bool sorted = false, std::function<bool(const T&)> nextPredicate = nullptr) : 
            maxSize(maxSize),
            sorted(sorted),
            nextPredicate(nextPredicate)
        {
        }

        int GetSize() const { return queue.size(); }
        

        /// Add object to tail (produce).
        void Push(const T& obj)
        {
            {
                // lock while adding object to queue
                std::unique_lock<std::mutex> lk(queueLock);

                // wait until space is available (or it's a priority item)
                while (maxSize > 0 && queue.size() >= maxSize-1 && (!nextPredicate || !nextPredicate(obj)))
                {
                    // yield timeslice
                    monitor.wait_for(lk, chrono::milliseconds(16));
                }

                if (sorted)
                {
                    // insert at the right position
                    Queue::iterator it = queue.begin();
                    for (; it != queue.end() && (*it) < obj; ++it);
                    
                    queue.insert(it, obj);
                }
                else
                {
                    // insert at the tail
                    queue.push_back(obj);
                }
            }
        }
        
        /// Get and remove from head (consume). Waits, while queue is empty.
        T Pop()
        {
            {
                // lock while removing object from queue
                std::unique_lock<std::mutex> lk(queueLock);

                // wait until the right thing is available
                while (queue.empty() || !ContainsValidElement())
                {
                    // yield timeslice
                    monitor.wait_for(lk, chrono::milliseconds(16));
                }
                
                T obj = queue.front();
                queue.pop_front();
                
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

                queue.clear();
            }
        }
    };
}

#endif // UTIL_THREADUTIL