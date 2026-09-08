#pragma once

namespace kailux
{
    template<uint32_t StableFrames = 4, uint32_t PixelThreshold = 4>
    class ResizeDebouncer
    {
    public:
        void Request(glm::ivec2 extent)
        {
            if (differs(mPendingExtent, extent))
            {
                mPendingExtent = extent;
                mStableFrames = {};
                return;
            }
            if (mStableFrames < StableFrames)
                ++mStableFrames;
        }

        std::optional<glm::ivec2> Poll(glm::ivec2 current) const
        {
            if (mStableFrames < StableFrames)
                return std::nullopt;
            if (!mPendingExtent.x || !mPendingExtent.y)
                return std::nullopt;
            if (!differs(current, mPendingExtent))
                return std::nullopt;
            return mPendingExtent;
        }

    private:
        static constexpr bool differs(glm::ivec2 a, glm::ivec2 b)
        {
            const auto dw = static_cast<int>(b.x)  - static_cast<int>(a.x);
            const auto dh = static_cast<int>(b.x) - static_cast<int>(a.x);
            return std::abs(dw) > static_cast<int>(PixelThreshold) ||
                   std::abs(dh) > static_cast<int>(PixelThreshold);
        }

        glm::ivec2 mPendingExtent{};
        uint32_t   mStableFrames{};
    };
}