#pragma once
#include "Context.h"
#include "Core.h"
#include "texture/Texture.h"

namespace kailux
{
    class SimulationView
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(SimulationView)

        static SimulationView create(const Context& context,
                                     vk::Format colorFormat,
                                     vk::Format depthFormat,
                                     vk::Extent2D extent,
                                     vk::SampleCountFlagBits samples);

        const Texture& GetColorTexture()    const;
        const Texture& GetDepthTexture()    const;
        const Texture& GetResolvedTexture() const;

        vk::Extent2D GetExtent()  const;
        ImTextureID  GetTextureId() const;
        void SetTextureId(ImTextureID id);

    private:
        Texture      mColor;
        Texture      mDepth;
        Texture      mResolved;
        vk::Extent2D mExtent{};
        ImTextureID  mTextureId{};
    };
}
