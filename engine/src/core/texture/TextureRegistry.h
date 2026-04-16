#pragma once
#include <assimp/material.h>

#include "ImageLoader.h"
#include "Texture.h"
#include "core/Context.h"
#include "core/Core.h"

namespace kailux
{
    struct TextureSetHandle
    {
        static constexpr uint32_t s_InvalidIndex = ~0u;

        uint32_t index = s_InvalidIndex;

        constexpr bool valid() const { return index != s_InvalidIndex; }
    };

    struct TextureSet
    {
        Shared<Texture> albedo;
        Shared<Texture> normal;
        Shared<Texture> roughness;
        Shared<Texture> metallic;
        Shared<Texture> ao;
    };

    enum class TextureType
    {
        Albedo = aiTextureType_BASE_COLOR,
        Normal = aiTextureType_NORMALS,
        Roughness = aiTextureType_DIFFUSE_ROUGHNESS,
        Metallic = aiTextureType_METALNESS,
        AO = aiTextureType_AMBIENT_OCCLUSION
    };

    class TextureRegistry
    {
    public:
        static constexpr uint32_t s_TextureTypes = 5;

        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(TextureRegistry)

        static TextureRegistry create(const Context& context, uint32_t meshCount);

        TextureSetHandle  registerTextureSet(const TextureSet& set);
        void              unregisterTextureSet(TextureSetHandle handle);
        void              updateTextureSet(TextureSetHandle handle, const TextureSet& set);
        const TextureSet& view(TextureSetHandle handle) const;
        TextureSetHandle  getDefaultSetHandle() const;

        struct MaterialData
        {
            std::string albedoPath;
            std::string normalPath;
            std::string roughnessPath;
            std::string metallicPath;
            std::string aoPath;
        };

        TextureSet createSetFromMaterialData(const Context &context, const MaterialData& data) const;

    private:
        void     allocResources(uint32_t meshCount);
        void     createDefaultTextures(const Context& context);

        uint32_t acquireSlot();

        TextureSet                   m_DefaultSet;
        TextureSetHandle             m_DefaultSetHandle;
        std::vector<TextureSet>      m_TexturePool;
        std::deque<uint32_t>         m_FreeSlots;
    };
}
