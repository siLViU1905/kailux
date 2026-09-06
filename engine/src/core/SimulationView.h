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
                                     glm::ivec2 extent,
                                     vk::SampleCountFlagBits samples);

        const Texture& GetColorTexture()    const;
        const Texture& GetDepthTexture()    const;
        const Texture& GetResolvedTexture() const;

        glm::ivec2 GetExtent()  const;
        ImTextureID  GetTextureId() const;
        void SetTextureId(ImTextureID id);

    private:
        Texture      mColor;
        Texture      mDepth;
        Texture      mResolved;
        glm::ivec2 mExtent{};
        ImTextureID  mTextureId{};
    };
}
