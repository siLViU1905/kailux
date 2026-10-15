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
       KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ThreadDispatcher)

        static ThreadDispatcher create(uint32_t threads);

        template<typename Func, typename... Args>
        auto Enqueue(Func &&func, Args &&... args)
        {
            using ReturnType = std::invoke_result_t<Func, Args...>;

            std::packaged_task<ReturnType()> task{
                std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
            };

            auto result{task.get_future()};
            {
                std::lock_guard lock{mState->mutex};
                mState->tasks.emplace_back(std::move(task));
            }
            mState->condition.notify_one();
            return result;
        }

        ~ThreadDispatcher();

    private:
        using Task = std::move_only_function<void()>;

        struct State
        {
            std::mutex                  mutex;
            std::condition_variable_any condition;
            std::deque<Task>            tasks;
            std::vector<std::jthread>   workers;

            void WorkerLoop(std::stop_token stopToken);
        };

        Scoped<State> mState;
    };
}
