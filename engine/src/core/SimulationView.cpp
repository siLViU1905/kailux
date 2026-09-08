#include "SimulationView.h"

#include "texture/TextureAllocator.h"

namespace kailux
{
    SimulationView::SimulationView() = default;

    SimulationView::SimulationView(SimulationView &&other) noexcept : mColor(std::move(other.mColor)),
                                                                      mDepth(std::move(other.mDepth)),
                                                                      mResolved(std::move(other.mResolved)),
                                                                      mExtent(other.mExtent),
                                                                      mTextureId(std::exchange(other.mTextureId, {}))
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
            mTextureId = std::exchange(other.mTextureId, {});
        }
        return *this;
    }

    SimulationView SimulationView::create(const Context &context, vk::Format colorFormat, vk::Format depthFormat,
        glm::ivec2 extent, vk::SampleCountFlagBits samples)
    {
        SimulationView view;
        view.mExtent = extent;

        view.mColor = TextureAllocator::create_empty(
            context, extent.x, extent.y, colorFormat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
            vk::ImageAspectFlagBits::eColor,
            samples
        );

        view.mDepth = TextureAllocator::create_empty(
            context, extent.x, extent.y, depthFormat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
            vk::ImageAspectFlagBits::eDepth,
            samples
        );

        view.mResolved = TextureAllocator::create_empty(
            context, extent.x, extent.y, colorFormat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            vk::ImageAspectFlagBits::eColor,
            vk::SampleCountFlagBits::e1
        );

        return view;
    }

    const Texture & SimulationView::GetColorTexture() const
    {
        return mColor;
    }

    const Texture & SimulationView::GetDepthTexture() const
    {
        return mDepth;
    }

    const Texture & SimulationView::GetResolvedTexture() const
    {
        return mResolved;
    }

    glm::ivec2 SimulationView::GetExtent() const
    {
        return mExtent;
    }

    ImTextureID SimulationView::GetTextureId() const
    {
        return mTextureId;
    }

    void SimulationView::SetTextureId(ImTextureID id)
    {
        mTextureId = id;
    }
}
