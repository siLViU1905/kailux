#pragma once
#include "../Context.h"
#include "../Core.h"

namespace kailux
{
    class ShadowMap
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ShadowMap)

        static ShadowMap create(
            const Context &context,
            uint32_t resolution,
            uint32_t layerCount,
            vk::Format format);

        vk::Image     GetImage() const;
        vk::ImageView GetArrayView() const;
        vk::ImageView GetLayerView(uint32_t layer) const;
        vk::Sampler   GetSampler() const;

        uint32_t      GetResolution() const;
        uint32_t      GetLayerCount() const;
        vk::Extent2D  GetExtent() const;

    private:
        void CreateImage(const Context &context, uint32_t resolution, uint32_t layerCount, vk::Format format);
        void CreateViews(const Context &context, uint32_t layerCount, vk::Format format);
        void CreateSampler(const Context &context);

        vk::raii::Image                  mImage{nullptr};
        vk::raii::DeviceMemory           mMemory{nullptr};
        vk::raii::ImageView              mArrayView{nullptr};
        std::vector<vk::raii::ImageView> mLayerViews;
        vk::raii::Sampler                mSampler{nullptr};

        uint32_t mResolution{};
        uint32_t mLayerCount{};
    };
}
