#include "ThreadDispatcher.h"

#include "core/Logger.h"

namespace kailux
{
    Shared<ThreadDispatcher> ThreadDispatcher::get()
    {
        static Shared<ThreadDispatcher> dispatcher(new ThreadDispatcher(s_MaxThreads));
        return dispatcher;
    }

    ThreadDispatcher::~ThreadDispatcher()
    {
        for (auto& worker : m_Workers)
            worker.request_stop();

        m_Condition.notify_all();
        m_Workers.clear();
    }

    ThreadDispatcher::ThreadDispatcher(uint32_t threads):m_Threads(threads)
    {
        KAILUX_LOG_PARENT_CLR_YELLOW("[ThreadDispatcher]")
        createWorkers();
        KAILUX_LOG_CHILD_CLR_YELLOW(std::format("Created dispatcher with {} threads", m_Workers.size()))
    }

    void ThreadDispatcher::createWorkers()
    {
        m_Workers.reserve(m_Threads);
        for (uint32_t i = 0; i < m_Threads; ++i)
            m_Workers.emplace_back([this](std::stop_token stopToken)
            {
                workerLoop(stopToken);
            });
    }

    void ThreadDispatcher::workerLoop(std::stop_token stopToken)
    {
        while (!stopToken.stop_requested())
        {
            Task task; {
                std::unique_lock lock(m_WaitMutex);
                m_Condition.wait(lock, [this, &stopToken]()
                {
                    return stopToken.stop_requested() || !m_Tasks.empty();
                });

                if (stopToken.stop_requested() && m_Tasks.empty())
                    return;

                task = std::move(m_Tasks.front());
                m_Tasks.pop_front();
            }
            task();
        }
    }
}
