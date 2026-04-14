#pragma once
#include <mutex>
#include "../Core.h"

namespace kailux
{
    template<typename T>
    class Queue
    {
    public:
        Queue() : m_Mutex(create_scoped<std::mutex>())
        {
        }

        Queue(const Queue &) = delete;

        Queue &operator=(const Queue &) = delete;

        Queue(Queue &&other) noexcept
            : m_Queue(std::move(other.m_Queue)),
              m_Mutex(std::move(other.m_Mutex))
        {
        }

        Queue &operator=(Queue &&other) noexcept
        {
            if (this != &other)
            {
                m_Queue = std::move(other.m_Queue);
                m_Mutex = std::move(other.m_Mutex);
            }
            return *this;
        }

        void push(const T& val)
        {
            std::lock_guard lock(*m_Mutex);
            m_Queue.push_front(val);
        }
        void push(T&& val)
        {
            std::lock_guard lock(*m_Mutex);
            m_Queue.push_front(std::move(val));
        }

        using PopResult = std::optional<T>;
        PopResult tryPop()
        {
            std::lock_guard lock(*m_Mutex);
            if (m_Queue.empty())
                return std::nullopt;
            auto val = std::move(m_Queue.back());
            m_Queue.pop_front();
            return val;
        }

    private:
        std::deque<T>          m_Queue;
        Scoped<std::mutex>     m_Mutex;
    };
}
