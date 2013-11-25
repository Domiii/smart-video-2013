
#include "workers.h"
#include <cassert>

namespace Util
{
    void Worker::Run()
    {
        assert(!isRunning);
        isRunning = true;

        /*Job * job;
        while (!isStopped && (job = taskQueue()))
        {
            (*job)();
        }*/

        isRunning = false;
        isStopped = false;
    }
}