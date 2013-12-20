
#include "Workers.h"
#include <cassert>
#include <numeric>

using namespace std;

namespace Util
{
    bool Worker::IsIdle() const { return !isRunning && !pool.IsStopped(); }

    void Worker::RunLoop()
    {   
        while (!pool.isStopped)
        {
            while (!pool.isStopped && pool.job && isRunning)
            {
                isRunning = pool.job(pool.iNextJobIndex++);
            }
            
            isRunning = false;

            this_thread::sleep_for(std::chrono::milliseconds(WorkerRunDelayMS));
        }
        pool.RemoveWorker(*this);
    }


    void WorkerPool::RemoveWorker(Worker& worker)
    {
        {
            // lock while removing
            unique_lock<mutex> lk(poolLock);
            
            // hack around
            // Must not use remove() here because that requires a copy operation in the VS 2012 std implementation
            // and Worker does not support copying.
            workers.remove_if([&worker, this](const Worker& that) { 
                if (worker == that)
                {
                    --nWorkerCount;
                    return true;
                }
                return false;
            });

            if (nWorkerCount == 0)
            {
                sleepCondition.notify_all();
            }
        }
    }


    void WorkerPool::AddWorkers(Job job)
    {
        int nWorkers = std::thread::hardware_concurrency();
        if (!nWorkers)
            nWorkers  = NMaxWorkersDefault;           // if the system does not reveal the amount, assign default
        AddWorkers(nWorkers, job);
    }

    
    void WorkerPool::AddWorkers(int nWorkers, Job job)
    {
        {
            // lock while adding threads and assigning tasks
            std::unique_lock<std::mutex> lk(poolLock);
            SetJob(job);
            isStopped = false;
            iNextJobIndex.store(0);

            // Add new workers
            for (int i = 0; i < nWorkers; ++i)
            {
                workers.emplace_front(*this, iNextWorkerId++);
            }
            nWorkerCount += nWorkers;
        }
    }


    void WorkerPool::Stop()
    {
        isStopped = true;
    }


    void WorkerPool::Join()
    {
        {
            // lock for join
            std::unique_lock<std::mutex> lk(poolLock);
            if (nWorkerCount > 0)
            {
                sleepCondition.wait(lk, [this]() { return nWorkerCount == 0; });
            }
        }
    }
}