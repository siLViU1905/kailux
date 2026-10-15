#include "ThreadDispatcher.h"

#include "core/Log.h"

namespace kailux
{
    ThreadDispatcher::ThreadDispatcher() = default;

    ThreadDispatcher::ThreadDispatcher(ThreadDispatcher &&) noexcept = default;

    ThreadDispatcher & ThreadDispatcher::operator=(ThreadDispatcher &&) noexcept = default;

    ThreadDispatcher ThreadDispatcher::create(uint32_t threads)
    {
        ThreadDispatcher dispatcher;
        dispatcher.mState = create_scoped<State>();

        auto& state{*dispatcher.mState};
        state.workers.reserve(threads);
        for (uint32_t i{}; i < threads; ++i)
            state.workers.emplace_back([&state](auto stopToken)
            {
                state.WorkerLoop(stopToken);
            });

        log::console.Debug("thread dispatcher: created with {} threads", threads);
        return dispatcher;
    }

    ThreadDispatcher::~ThreadDispatcher() = default;

    void ThreadDispatcher::State::WorkerLoop(std::stop_token stopToken)
    {
        while (true)
        {
            Task task;
            {
                std::unique_lock lock{mutex};
                if (!condition.wait(lock, stopToken, [this]()
                {
                    return !tasks.empty();
                }))
                    return;

                task = std::move(tasks.front());
                tasks.pop_front();
            }
            task();
        }
    }
}
