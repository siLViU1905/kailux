#pragma once
#include <mutex>
#include "../Core.h"

namespace kailux
{
    template<typename T, size_t Capacity = 1024>
    class Queue
    {
    public:
        Queue() = default;
        Queue(const Queue&) = delete;
        Queue& operator=(const Queue&) = delete;

        Queue(Queue &&other) noexcept
            : m_Buffer(std::move(other.m_Buffer)),
              m_Head(other.m_Head.load(std::memory_order_relaxed)),
              m_Tail(other.m_Tail.load(std::memory_order_relaxed))
        {
        }

        Queue &operator=(Queue &&other) noexcept
        {
            if (this != &other)
            {
                m_Buffer = std::move(other.m_Buffer);
                m_Head.store(other.m_Head.load(std::memory_order_relaxed),
                             std::memory_order_relaxed);
                m_Tail.store(other.m_Tail.load(std::memory_order_relaxed),
                             std::memory_order_relaxed);
            }
            return *this;
        }

        void push(const T& val)
        {
            auto tail = m_Tail.load(std::memory_order_relaxed);
            m_Buffer[tail & s_Mask] = val;
            m_Tail.store(tail + 1, std::memory_order_release);
        }
        void push(T&& val)
        {
            auto tail = m_Tail.load(std::memory_order_relaxed);
            m_Buffer[tail & s_Mask] = std::move(val);
            m_Tail.store(tail + 1, std::memory_order_release);
        }
        template<typename... Args>
        void emplace(Args&&... args)
        {
            auto tail = m_Tail.load(std::memory_order_relaxed);
            m_Buffer[tail & s_Mask] = T(std::forward<Args>(args)...);
            m_Tail.store(tail + 1, std::memory_order_release);
        }

        using PopResult = std::optional<T>;
        PopResult tryPop()
        {
            auto head = m_Head.load(std::memory_order_relaxed);
            if (head == m_Tail.load(std::memory_order_acquire))
                return std::nullopt;
            auto val = std::move(m_Buffer[head & s_Mask]);
            m_Head.store(head + 1, std::memory_order_release);
            return val;
        }

    private:
        static constexpr size_t s_Mask = Capacity - 1;

        std::vector<T> m_Buffer{Capacity};

        std::atomic<size_t> m_Head{0};
        std::atomic<size_t> m_Tail{0};
    };
}
