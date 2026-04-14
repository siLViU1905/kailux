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
        inline static uint32_t s_MaxThreads = 0;

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
                std::lock_guard lock(m_WaitMutex);
                m_Tasks.emplace_back([this, task]()
                {
                    (*task)();
                });
            }
            m_Condition.notify_one();
            return result;
        }

    private:
        ThreadDispatcher(uint32_t threads);

        void createWorkers();

        void workerLoop(std::stop_token stopToken);

        uint32_t                  m_Threads;
        using Task = std::move_only_function<void()>;
        std::vector<std::jthread> m_Workers;
        std::deque<Task>          m_Tasks;
        std::mutex                m_WaitMutex;
        std::condition_variable   m_Condition;
    };
}
