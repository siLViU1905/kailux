#include "RenderTarget.h"

#include "texture/TextureAllocator.h"

namespace kailux
{
    RenderTarget::RenderTarget() = default;

    RenderTarget::RenderTarget(RenderTarget &&other) noexcept : mInfo(other.mInfo),
                                                                mColor(std::move(other.mColor)),
                                                                mDepth(std::move(other.mDepth)),
                                                                mResolved(std::move(other.mResolved)),
                                                                mId(std::move(other.mId)),
                                                                mResolvedId(std::move(other.mResolvedId)),
                                                                mTextureId(other.mTextureId)
    {
    }

    RenderTarget & RenderTarget::operator=(RenderTarget &&other) noexcept
    {
        if (this != &other)
        {
            mInfo = other.mInfo;
            mColor = std::move(other.mColor);
            mDepth = std::move(other.mDepth);
            mResolved = std::move(other.mResolved);
            mId = std::move(other.mId);
            mResolvedId = std::move(other.mResolvedId);
            mTextureId = other.mTextureId;
        }
        return *this;
    }

    RenderTarget RenderTarget::create(const Context &context, const Info &info)
    {
        RenderTarget target;
        target.mInfo = info;

        target.CreateNoIdTextures(context, info);

        if (info.withIdTexture)
            target.CreateIdTextures(context, info);

        return target;
    }

    const Texture & RenderTarget::GetColorTexture() const
    {
        return mColor;
    }

    const Texture & RenderTarget::GetDepthTexture() const
    {
        return mDepth;
    }

    const Texture & RenderTarget::GetPresentedTexture() const
    {
        return IsMultisampled() ? mResolved : mColor;
    }

    vk::ImageView RenderTarget::GetResolveView() const
    {
        return IsMultisampled() ? mResolved.GetImageView() : vk::ImageView{};
    }

    bool RenderTarget::IsMultisampled() const
    {
        return mInfo.samples != vk::SampleCountFlagBits::e1;
    }

    bool RenderTarget::HasIdTexture() const
    {
        return mInfo.withIdTexture;
    }

    const Texture & RenderTarget::GetIdTexture() const
    {
        return mId;
    }

    const Texture & RenderTarget::GetResolvedIdTexture() const
    {
        return mResolvedId;
    }

    const Texture & RenderTarget::GetReadableIdTexture() const
    {
        return IsMultisampled() ? mResolvedId : mId;
    }

    const RenderTarget::Info & RenderTarget::GetInfo() const
    {
        return mInfo;
    }

    glm::ivec2 RenderTarget::GetExtent() const
    {
        return mInfo.extent;
    }

    vk::Extent2D RenderTarget::GetVkExtent() const
    {
        return {
            static_cast<uint32_t>(mInfo.extent.x),
            static_cast<uint32_t>(mInfo.extent.y)
        };
    }

    ImTextureID RenderTarget::GetTextureId() const
    {
        return mTextureId;
    }

    void RenderTarget::SetTextureId(ImTextureID id)
    {
        mTextureId = id;
    }

    void RenderTarget::CreateNoIdTextures(const Context &context, const Info &info)
    {
        const auto width{static_cast<uint32_t>(info.extent.x)};
        const auto height{static_cast<uint32_t>(info.extent.y)};

        const bool multisampled{info.samples != vk::SampleCountFlagBits::e1};

        mColor = TextureAllocator::create_empty(
            context, width, height, info.colorFormat,
            multisampled
                ? vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment
                : vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            vk::ImageAspectFlagBits::eColor,
            info.samples
        );

        if (multisampled)
            mResolved = TextureAllocator::create_empty(
                context, width, height, info.colorFormat,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                vk::ImageAspectFlagBits::eColor,
                vk::SampleCountFlagBits::e1
            );

        mDepth = TextureAllocator::create_empty(
            context, width, height, info.depthFormat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
            vk::ImageAspectFlagBits::eDepth,
            info.samples
        );
    }

    void RenderTarget::CreateIdTextures(const Context &context, const Info &info)
    {
        const auto width{static_cast<uint32_t>(info.extent.x)};
        const auto height{static_cast<uint32_t>(info.extent.y)};

        const bool multisampled{info.samples != vk::SampleCountFlagBits::e1};

        constexpr auto kIdFormat{vk::Format::eR32Uint};
        constexpr auto kReadableUsage{
            vk::ImageUsageFlagBits::eColorAttachment
            | vk::ImageUsageFlagBits::eStorage
            | vk::ImageUsageFlagBits::eSampled
        };

        mId = TextureAllocator::create_empty(
            context, width, height, kIdFormat,
            multisampled
                ? vk::ImageUsageFlagBits::eColorAttachment
                : kReadableUsage,
            vk::ImageAspectFlagBits::eColor,
            info.samples
        );

        if (multisampled)
            mResolvedId = TextureAllocator::create_empty(
                context, width, height, kIdFormat,
                kReadableUsage,
                vk::ImageAspectFlagBits::eColor,
                vk::SampleCountFlagBits::e1
            );
    }
}
