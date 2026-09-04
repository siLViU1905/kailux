#pragma once

namespace kailux
{
    template<uint32_t StableFrames = 4, uint32_t PixelThreshold = 4>
    class ResizeDebouncer
    {
    public:
        void request(vk::Extent2D extent)
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

        std::optional<vk::Extent2D> poll(vk::Extent2D current) const
        {
            if (mStableFrames < StableFrames)
                return std::nullopt;
            if (!mPendingExtent.width || !mPendingExtent.height)
                return std::nullopt;
            if (!differs(current, mPendingExtent))
                return std::nullopt;
            return mPendingExtent;
        }

    private:
        static constexpr bool differs(vk::Extent2D a, vk::Extent2D b)
        {
            const auto dw = static_cast<int>(b.width)  - static_cast<int>(a.width);
            const auto dh = static_cast<int>(b.height) - static_cast<int>(a.height);
            return std::abs(dw) > static_cast<int>(PixelThreshold) ||
                   std::abs(dh) > static_cast<int>(PixelThreshold);
        }

        vk::Extent2D mPendingExtent{};
        uint32_t     mStableFrames{};
    };
}