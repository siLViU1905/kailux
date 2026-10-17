#pragma once
#include "Context.h"
#include "texture/Texture.h"

namespace kailux
{
    class RenderTarget
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(RenderTarget)

        struct Info
        {
            vk::Format              colorFormat{};
            vk::Format              depthFormat{};
            glm::ivec2              extent{};
            vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
            bool                    withIdTexture{};
        };

        static RenderTarget create(const Context &context, const Info &info);

        const Texture &GetColorTexture() const;
        const Texture &GetDepthTexture() const;
        const Texture &GetPresentedTexture() const;

        vk::ImageView GetResolveView() const;

        bool IsMultisampled() const;
        bool HasIdTexture()   const;

        const Texture &GetIdTexture()         const;
        const Texture &GetResolvedIdTexture() const;
        const Texture &GetReadableIdTexture() const;

        const Info &GetInfo()   const;
        glm::ivec2  GetExtent() const;

        vk::Extent2D GetVkExtent() const;

        ImTextureID GetTextureId() const;
        void        SetTextureId(ImTextureID id);

    private:
        void CreateNoIdTextures(const Context &context, const Info& info);
        void CreateIdTextures(const Context &context, const Info& info);

        Info        mInfo{};
        Texture     mColor;
        Texture     mDepth;
        Texture     mResolved;
        Texture     mId;
        Texture     mResolvedId;
        ImTextureID mTextureId{};
    };
}
