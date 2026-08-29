#include "SimulationView.h"

#include "texture/TextureAllocator.h"

namespace kailux
{
    SimulationView::SimulationView() = default;

    SimulationView::SimulationView(SimulationView &&other) noexcept : mColor(std::move(other.mColor)),
                                                                      mDepth(std::move(other.mDepth)),
                                                                      mResolved(std::move(other.mResolved)),
                                                                      mExtent(other.mExtent),
                                                                      mTextureId(other.mTextureId)
    {
    }

    SimulationView & SimulationView::operator=(SimulationView &&other) noexcept
    {
        if (this != &other)
        {
            mColor = std::move(other.mColor);
            mDepth = std::move(other.mDepth);
            mResolved = std::move(other.mResolved);
            mExtent = other.mExtent;
            mTextureId = other.mTextureId;
        }
        return *this;
    }

    SimulationView SimulationView::create(const Context &context, vk::Format colorFormat, vk::Format depthFormat,
        vk::Extent2D extent, vk::SampleCountFlagBits samples)
    {
        SimulationView view;
        view.mExtent = extent;

        view.mColor = TextureAllocator::create_empty(
            context, extent.width, extent.height, colorFormat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
            vk::ImageAspectFlagBits::eColor,
            samples
        );

        view.mDepth = TextureAllocator::create_empty(
            context, extent.width, extent.height, depthFormat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
            vk::ImageAspectFlagBits::eDepth,
            samples
        );

        view.mResolved = TextureAllocator::create_empty(
            context, extent.width, extent.height, colorFormat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            vk::ImageAspectFlagBits::eColor,
            vk::SampleCountFlagBits::e1
        );

        return view;
    }

    const Texture & SimulationView::getColorTexture() const
    {
        return mColor;
    }

    const Texture & SimulationView::getDepthTexture() const
    {
        return mDepth;
    }

    const Texture & SimulationView::getResolvedTexture() const
    {
        return mResolved;
    }

    vk::Extent2D SimulationView::getExtent() const
    {
        return mExtent;
    }

    ImTextureID SimulationView::getTextureId() const
    {
        return mTextureId;
    }

    void SimulationView::setTextureId(ImTextureID id)
    {
        mTextureId = id;
    }
}
