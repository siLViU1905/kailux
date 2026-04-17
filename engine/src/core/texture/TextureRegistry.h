#pragma once
#include <assimp/material.h>
#include <magic_enum/magic_enum.hpp>

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
        static constexpr std::array s_TextureTypes = magic_enum::enum_values<TextureType>();

        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(TextureRegistry)

        static TextureRegistry create(const Context& context, uint32_t meshCount);

        TextureSetHandle  registerTextureSet(const TextureSet& set);
        void              unregisterTextureSet(TextureSetHandle handle);
        void              updateTextureSet(TextureSetHandle handle, const TextureSet& set);
        const TextureSet& view(TextureSetHandle handle) const;
        TextureSetHandle  getDefaultSetHandle() const;

        struct MaterialData
        {
            ImageLoader::ImageData albedoData;
            ImageLoader::ImageData normalData;
            ImageLoader::ImageData roughnessData;
            ImageLoader::ImageData metallicData;
            ImageLoader::ImageData aoData;
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
