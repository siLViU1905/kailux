#pragma once
#include <future>
#include <mutex>
#include <thread>

#include "core/Core.h"

namespace kailux
{
    class ThreadDispatcher
    {
    public:
        inline static uint32_t kMaxThreads = 0;

        KAILUX_DECLARE_SINGLETON(ThreadDispatcher)
        ~ThreadDispatcher();

        template<typename Func, typename... Args>
        auto enqueue(Func &&func, Args &&... args)
        {
            using ReturnType = std::invoke_result_t<Func, Args...>;

            auto task = create_shared<std::packaged_task<ReturnType()> >
            (
                std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
            );

            auto result = task->get_future();
            {
                std::lock_guard lock(mWaitMutex);
                mTasks.emplace_back([this, task]()
                {
                    (*task)();
                });
            }
            mCondition.notify_one();
            return result;
        }

    private:
        ThreadDispatcher(uint32_t threads);

        void createWorkers();

        void workerLoop(std::stop_token stopToken);

        uint32_t                  mThreads;
        using Task = std::move_only_function<void()>;
        std::vector<std::jthread> mWorkers;
        std::deque<Task>          mTasks;
        std::mutex                mWaitMutex;
        std::condition_variable   mCondition;
    };
}
