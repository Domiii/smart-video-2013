#ifndef UTIL_WORKER_H
#define UTIL_WORKER_H

#include "ThreadUtil.h"

// Misc stuff
#include <functional>
#include <unordered_map>
#include <memory>

// Threading
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <forward_list>
#include <queue>


namespace Util
{
    typedef uint32 JobIndex;

    typedef std::function<bool(JobIndex)> Job;
    
    class WorkerPool;

    /// Basic thread wrapper as part of a pool.
    class Worker
    {
        /// Time to wait periodically while idle (in ms).
        /// TODO: Use wait/notify instead.
        static const int WorkerRunDelayMS = 50;
        
        friend class WorkerPool;
        
        /// id of this worker
        int workerId;
        bool isRunning;
        
        WorkerPool& pool;
        std::thread thread;

        /// Thread loop.
        void RunLoop();

        /// Disallow copy ctor
        Worker(const Worker&);
        Worker& operator=(const Worker&);

    public:
        Worker(WorkerPool& pool, int workerId) :
            workerId(workerId),
            isRunning(true),        // set to running initially, so Join() won't return immediately
            pool(pool),
            thread(std::bind(&Worker::RunLoop, this))
        {
            thread.detach();
        }


        virtual ~Worker() 
        {
        }

        bool operator==(const Worker& other)
        {
            return other.workerId == workerId;
        }
        
        /// Unique id assigned to this worker.
        int GetId() const { return workerId; }

        /// Whether this worker is currently running.
        bool IsRunning() const { return isRunning; }

        /// Whether this worker is hungry for more work.
        bool IsIdle() const;
    };




    /// A pool of workers to work on one or multiple tasks in parallel.
    class WorkerPool
    {
        friend class Worker;

        // If the system does not reveal the amount, use this as default.
        static const uint32 NMaxWorkersDefault = 4;
        
        std::mutex poolLock;
        std::condition_variable sleepCondition;
        std::atomic<int> iNextWorkerId;
        std::forward_list<Worker> workers;
        int nWorkerCount;
        
        std::atomic<JobIndex> iNextJobIndex;
        Job job;
        bool isStopped;

        /// Removes the given worker from this pool.
        void RemoveWorker(Worker& worker);

    public:
        WorkerPool() :
            isStopped(false),
            nWorkerCount(0)
        {
        }

        virtual ~WorkerPool()
        {
            Stop();
            Join();
        }

        /// Whether this pool is stopped. 
        /// If the pool is stopped, all threads will be disposed at earliest convenience.
        bool IsStopped() const { return isStopped; }

        /// Sets the current task queue and adds a system-default amount of workers.
        void AddWorkers(Job job);

        /// Sets the current task queue and adds nWorkers.
        void AddWorkers(int nWorkers, Job job);
        
        /// Sets the current task queue
        void SetJob(Job job) { this->job = job; }

        /// Stops all threads, clears the worker queue and disposes all threads.
        /// This call is non-blocking. Call Join to wait for all threads to stop.
        /// Note: Another call to start will clear the worker queue.
        void Stop();

        /// Wait until all threads in pool are idle or disposed.
        void Join();
    };
}


#endif // UTIL_WORKER_H