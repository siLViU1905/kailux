#pragma once

#include <vector>

namespace kailux
{
    template<uint32_t Delay>
    class DeferredResourceEraser
    {
    public:
        using Task = std::move_only_function<void()>;
        void enqueue(Task&& task)
        {
            m_Tasks.emplace_back(std::move(task), Delay);
        }

        void tick()
        {
            std::erase_if(m_Tasks, [](auto &pending)
            {
                --pending.remainingFrames;
                if (pending.remainingFrames == 0)
                {
                    pending.task();
                    return true;
                }
                return false;
            });
        }

    private:
        struct PendingTask
        {
            Task     task;
            uint32_t remainingFrames{Delay};
        };

        std::vector<PendingTask> m_Tasks;
    };
}