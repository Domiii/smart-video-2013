#ifndef UTIL_WORKER_H
#define UTIL_WORKER_H

#include <thread>
#include <mutex>
#include <condition_variable>

#include <functional>
#include <unordered_map>
#include <memory>

namespace Util
{
    typedef std::function<void(void)> Job;
    typedef std::function<Job(void)> JobGenerator;

    class Worker
    {
        bool isRunning;
        bool isStopped;
        JobGenerator taskQueue;
        std::unique_ptr<std::thread> thread;

    public:
        Worker() :
            isRunning(false),
            isStopped(false)
        {
        }

        void SetTaskQueue(JobGenerator taskQueue)
        {
            this->taskQueue = taskQueue;
        }
        
        /// Whether this worker is currently running.
        bool IsRunning() const { return isRunning; }
        
        /// Whether this worker has been asked to stop.
        bool IsStopped() const { return isStopped; }

        /// Start processing the jobs.
        void Run();

        /// Stop processing (at earliest convinience).
        void Stop() { isStopped = true; }
    };

    /// A pool of workers to work on one or multiple tasks in parallel.
    class WorkerPool
    {
        std::vector<Worker> workers;

    public:
        /// Lets nWorkers work on the given tasks.
        /// Creates new workers, if there are not enough.
        void Start(int nWorkers, JobGenerator taskQueue);
    };
}


#endif // UTIL_WORKER_H