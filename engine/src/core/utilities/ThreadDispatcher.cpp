#include "ThreadDispatcher.h"

#include "core/Log.h"

namespace kailux
{
    Shared<ThreadDispatcher> ThreadDispatcher::get()
    {
        static Shared<ThreadDispatcher> dispatcher(new ThreadDispatcher(kMaxThreads));
        return dispatcher;
    }

    ThreadDispatcher::~ThreadDispatcher()
    {
        for (auto& worker : mWorkers)
            worker.request_stop();

        mCondition.notify_all();
        mWorkers.clear();
    }

    ThreadDispatcher::ThreadDispatcher(uint32_t threads):mThreads(threads)
    {
        KAILUX_LOG_PARENT_CLR_YELLOW("[ThreadDispatcher]")
        createWorkers();
        KAILUX_LOG_CHILD_CLR_YELLOW(std::format("Created dispatcher with {} threads", mWorkers.size()))
    }

    void ThreadDispatcher::createWorkers()
    {
        mWorkers.reserve(mThreads);
        for (uint32_t i = 0; i < mThreads; ++i)
            mWorkers.emplace_back([this](std::stop_token stopToken)
            {
                workerLoop(stopToken);
            });
    }

    void ThreadDispatcher::workerLoop(std::stop_token stopToken)
    {
        while (!stopToken.stop_requested())
        {
            Task task; {
                std::unique_lock lock(mWaitMutex);
                mCondition.wait(lock, [this, &stopToken]()
                {
                    return stopToken.stop_requested() || !mTasks.empty();
                });

                if (stopToken.stop_requested() && mTasks.empty())
                    return;

                task = std::move(mTasks.front());
                mTasks.pop_front();
            }
            task();
        }
    }
}
