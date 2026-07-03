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
            : mBuffer(std::move(other.mBuffer)),
              mHead(other.mHead.load(std::memory_order_relaxed)),
              mTail(other.mTail.load(std::memory_order_relaxed))
        {
        }

        Queue &operator=(Queue &&other) noexcept
        {
            if (this != &other)
            {
                mBuffer = std::move(other.mBuffer);
                mHead.store(other.mHead.load(std::memory_order_relaxed),
                             std::memory_order_relaxed);
                mTail.store(other.mTail.load(std::memory_order_relaxed),
                             std::memory_order_relaxed);
            }
            return *this;
        }

        void push(const T& val)
        {
            auto tail = mTail.load(std::memory_order_relaxed);
            mBuffer[tail & kMask] = val;
            mTail.store(tail + 1, std::memory_order_release);
        }
        void push(T&& val)
        {
            auto tail = mTail.load(std::memory_order_relaxed);
            mBuffer[tail & kMask] = std::move(val);
            mTail.store(tail + 1, std::memory_order_release);
        }
        template<typename... Args>
        void emplace(Args&&... args)
        {
            auto tail = mTail.load(std::memory_order_relaxed);
            mBuffer[tail & kMask] = T(std::forward<Args>(args)...);
            mTail.store(tail + 1, std::memory_order_release);
        }

        using PopResult = std::optional<T>;
        PopResult tryPop()
        {
            auto head = mHead.load(std::memory_order_relaxed);
            if (head == mTail.load(std::memory_order_acquire))
                return std::nullopt;
            auto val = std::move(mBuffer[head & kMask]);
            mHead.store(head + 1, std::memory_order_release);
            return val;
        }

    private:
        static constexpr size_t kMask = Capacity - 1;

        std::vector<T> mBuffer{Capacity};

        std::atomic<size_t> mHead{0};
        std::atomic<size_t> mTail{0};
    };
}
