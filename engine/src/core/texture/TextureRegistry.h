#pragma once
#include "ImageLoader.h"
#include "Texture.h"
#include "core/Context.h"
#include "core/Core.h"

namespace kailux
{
    struct TextureHandle
    {
        static constexpr uint32_t s_InvalidIndex = ~0u;

        uint32_t index = s_InvalidIndex;

        constexpr bool valid() const { return index != s_InvalidIndex; }
    };

    struct TextureSet
    {
        TextureHandle albedo;
        TextureHandle normal;
        TextureHandle roughness;
        TextureHandle metallic;
        TextureHandle ao;
    };

    class TextureRegistry
    {
    public:
        static constexpr uint32_t s_TextureTypes = 5;

        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(TextureRegistry)

        static TextureRegistry create(const Context& context, uint32_t meshCount);

        Shared<Texture>   view(TextureHandle handle) const;
        TextureHandle     registerTexture(Shared<Texture> texture);
        void              unregisterTexture(TextureHandle handle);
        uint32_t          acquireSlot();

        TextureSet        getDefaultSet() const;

    private:
        void allocResources(uint32_t meshCount);
        void createDefaultTextures(const Context& context);

        TextureSet                   m_DefaultSet;
        std::vector<Shared<Texture>> m_TexturePool;
        std::deque<uint32_t>         m_FreeSlots;
    };
}
