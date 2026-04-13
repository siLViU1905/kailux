#pragma once
#include <mutex>

namespace kailux
{
    template<typename T>
    class Queue
    {
    public:
        void push(const T& val)
        {
            std::lock_guard lock(m_Mutex);
            m_Queue.push_front(val);
        }
        void push(T&& val)
        {
            std::lock_guard lock(m_Mutex);
            m_Queue.push_front(std::move(val));
        }

        using PopResult = std::optional<T>;
        PopResult tryPop()
        {
            std::lock_guard lock(m_Mutex);
            if (m_Queue.empty())
                return std::nullopt;
            auto val = std::move(m_Queue.back());
            m_Queue.pop_front();
            return val;
        }

    private:
        std::deque<T> m_Queue;
        std::mutex     m_Mutex;
    };
}
