#pragma once

#include <vector>
#include  "profiling/Profiling.h"

namespace kailux
{
    template<uint32_t Delay>
    class DeferredResourceEraser
    {
    public:
        using Task = std::move_only_function<void()>;
        void Enqueue(Task&& task)
        {
            mTasks.emplace_back(std::move(task), Delay);
        }

        void Tick()
        {
            KAILUX_PROFILE_CPU(CpuSection::ResourceEraser);
            std::erase_if(mTasks, [](auto &pending)
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

        std::vector<PendingTask> mTasks;
    };
}