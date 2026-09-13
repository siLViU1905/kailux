#include "ShadowMap.h"

#include "../Log.h"

namespace kailux
{
    ShadowMap::ShadowMap() = default;

    ShadowMap::ShadowMap(ShadowMap &&other) noexcept : mImage(std::move(other.mImage)),
                                                       mMemory(std::move(other.mMemory)),
                                                       mArrayView(std::move(other.mArrayView)),
                                                       mLayerViews(std::move(other.mLayerViews)),
                                                       mCubeViews(std::move(other.mCubeViews)),
                                                       mSampler(std::move(other.mSampler)),
                                                       mResolution(other.mResolution),
                                                       mLayerCount(other.mLayerCount)
    {
    }

    ShadowMap & ShadowMap::operator=(ShadowMap &&other) noexcept
    {
        if (this != &other)
        {
            mImage = std::move(other.mImage);
            mMemory = std::move(other.mMemory);
            mArrayView = std::move(other.mArrayView);
            mLayerViews = std::move(other.mLayerViews);
            mCubeViews = std::move(other.mCubeViews);
            mSampler = std::move(other.mSampler);
            mResolution = other.mResolution;
            mLayerCount = other.mLayerCount;
        }
        return *this;
    }

    ShadowMap ShadowMap::create(const Context &context, uint32_t resolution, uint32_t layerCount, vk::Format format, bool createCube)
    {
        ShadowMap map;
        map.mResolution = resolution;
        map.mLayerCount = layerCount;

        map.CreateImage(context, resolution, layerCount, format, createCube);
        log::console.Debug("shadow_map: image created");

        map.CreateViews(context, layerCount, format, createCube);
        log::console.Debug("shadow_map: image view created");

        map.CreateSampler(context);
        log::console.Debug("shadow_map: sampler created");

        return map;
    }

    vk::Image ShadowMap::GetImage() const
    {
        return *mImage;
    }

    vk::ImageView ShadowMap::GetArrayView() const
    {
        return *mArrayView;
    }

    vk::ImageView ShadowMap::GetLayerView(uint32_t layer) const
    {
        assert(layer < mLayerViews.size() && "Shadow map layer out of range");
        return *mLayerViews[layer];
    }

    vk::ImageView ShadowMap::GetCubeView(uint32_t cube) const
    {
        assert(cube < mCubeViews.size() && "Shadow map cube out of range");
        return *mCubeViews[cube];
    }

    uint32_t ShadowMap::GetCubeCount() const
    {
        return static_cast<uint32_t>(mCubeViews.size());
    }

    vk::Sampler ShadowMap::GetSampler() const
    {
        return *mSampler;
    }

    uint32_t ShadowMap::GetResolution() const
    {
        return mResolution;
    }

    uint32_t ShadowMap::GetLayerCount() const
    {
        return mLayerCount;
    }

    vk::Extent2D ShadowMap::GetExtent() const
    {
        return {mResolution, mResolution};
    }

    void ShadowMap::CreateImage(const Context &context, uint32_t resolution, uint32_t layerCount, vk::Format format, bool createCube)
    {
        vk::ImageCreateInfo imageInfo{};
        if (createCube)
            imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;

        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.format = format;
        imageInfo.extent = vk::Extent3D{resolution, resolution, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = layerCount;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;

        mImage = vk::raii::Image(context.mDevice, imageInfo);

        const auto memReqs{mImage.getMemoryRequirements()};
        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = context.FindMemoryType(
            memReqs.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal
            );

        mMemory = vk::raii::DeviceMemory(context.mDevice, allocInfo);
        mImage.bindMemory(*mMemory, 0);
    }

    void ShadowMap::CreateViews(const Context &context, uint32_t layerCount, vk::Format format, bool createCube)
    {
        vk::ImageViewCreateInfo arrayInfo{};
        arrayInfo.image = *mImage;
        arrayInfo.viewType = vk::ImageViewType::e2DArray;
        arrayInfo.format = format;
        arrayInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        arrayInfo.subresourceRange.baseMipLevel = 0;
        arrayInfo.subresourceRange.levelCount = 1;
        arrayInfo.subresourceRange.baseArrayLayer = 0;
        arrayInfo.subresourceRange.layerCount = layerCount;

        mArrayView = vk::raii::ImageView(context.mDevice, arrayInfo);

        mLayerViews.clear();
        mLayerViews.reserve(layerCount);
        for (uint32_t layer{}; layer < layerCount; ++layer)
        {
            vk::ImageViewCreateInfo layerInfo{arrayInfo};
            layerInfo.viewType = vk::ImageViewType::e2DArray;
            layerInfo.subresourceRange.baseArrayLayer = layer;
            layerInfo.subresourceRange.layerCount = 1;

            mLayerViews.emplace_back(context.mDevice, layerInfo);
        }

        mCubeViews.clear();
        if (!createCube)
            return;

        const uint32_t cubeCount{layerCount / 6};
        mCubeViews.reserve(cubeCount);
        for (uint32_t i{}; i < cubeCount; ++i)
        {
            vk::ImageViewCreateInfo cubeInfo{arrayInfo};
            cubeInfo.viewType = vk::ImageViewType::eCube;
            cubeInfo.subresourceRange.baseArrayLayer = i * 6;
            cubeInfo.subresourceRange.layerCount = 6;

            mCubeViews.emplace_back(context.mDevice, cubeInfo);
        }
    }

    void ShadowMap::CreateSampler(const Context &context)
    {
        vk::SamplerCreateInfo samplerInfo{};
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToBorder;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToBorder;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToBorder;
        samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
        samplerInfo.unnormalizedCoordinates = vk::False;
        samplerInfo.compareEnable = vk::True;
        samplerInfo.compareOp = vk::CompareOp::eLessOrEqual;
        samplerInfo.mipLodBias = 0.f;
        samplerInfo.minLod = 0.f;
        samplerInfo.maxLod = 1.f;

        mSampler = vk::raii::Sampler(context.mDevice, samplerInfo);
    }
}
