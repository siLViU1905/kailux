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
        log::console.Debug("thread dispatcher: creating");
        CreateWorkers();
        log::console.Debug("thread dispatcher: created with {} threads", mWorkers.size());
    }

    void ThreadDispatcher::CreateWorkers()
    {
        mWorkers.reserve(mThreads);
        for (uint32_t i = 0; i < mThreads; ++i)
            mWorkers.emplace_back([this](std::stop_token stopToken)
            {
                WorkerLoop(stopToken);
            });
    }

    void ThreadDispatcher::WorkerLoop(std::stop_token stopToken)
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
