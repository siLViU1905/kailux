#pragma once
#include <assimp/material.h>
#include <magic_enum/magic_enum.hpp>

#include "ImageLoader.h"
#include "Texture.h"
#include "core/Context.h"
#include "core/Core.h"
#include "core/TransferManager.h"
#include "core/buffer/Buffer.h"
#include "core/components/gpu/MaterialSlot.h"

namespace kailux
{
    using TextureSetHandle = Handle;
    using TextureHandle = Handle;
    using MaterialHandle = Handle;

    enum class TextureType
    {
        Albedo = aiTextureType_BASE_COLOR,
        Normal = aiTextureType_NORMALS,
        Roughness = aiTextureType_DIFFUSE_ROUGHNESS,
        Metallic = aiTextureType_METALNESS,
        AO = aiTextureType_AMBIENT_OCCLUSION
    };

    struct AsyncMaterialResult
    {
        MaterialSlot               slot;
        std::vector<TextureHandle> handles;
        std::vector<ImageUpload>   uploads;
        std::vector<Buffer>        staging;
    };

    class TextureRegistry
    {
    public:
        static constexpr std::array kTextureTypes = magic_enum::enum_values<TextureType>();

        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(TextureRegistry)

        static TextureRegistry create(const Context &context,
                                      std::string_view directoryIconPath,
                                      std::string_view fileIconPath
        );

        std::optional<TextureHandle>  registerTexture(Texture&& texture);
        void                          releaseTexture(TextureHandle handle);
        const Texture&                getTexture(TextureHandle handle) const;
        std::optional<MaterialHandle> registerMaterial(const MaterialSlot& slot);
        void                          updateMaterial(MaterialHandle handle, const MaterialSlot& slot);
        void                          releaseMaterial(MaterialHandle handle);
        const MaterialSlot&           getMaterial(MaterialHandle handle) const;
        MaterialHandle                getDefaultMaterialHandle() const;
        TextureHandle                 getDefaultTextureHandle(TextureType type) const;

        std::span<const MaterialSlot> viewMaterials() const;

        const Texture& getAssetBrowserDirectoryIconTexture() const;
        const Texture& getAssetBrowserFileIconTexture() const;

        struct MaterialData
        {
            ImageLoader::ImageData albedoData;
            ImageLoader::ImageData normalData;
            ImageLoader::ImageData roughnessData;
            ImageLoader::ImageData metallicData;
            ImageLoader::ImageData aoData;
        };

        AsyncMaterialResult createMaterialFromData(const Context &context, const MaterialData& data);

    private:
        void     allocResources();
        void     createDefaultTextures(const Context& context);
        void     createAssetBrowserTextures(const Context& context, std::string_view directoryIconPath, std::string_view fileIconPath);

        std::vector<std::optional<Texture>>  mTextures;
        std::vector<uint32_t>                mTextureRefCount;
        std::deque<uint32_t>                 mFreeTextureSlots;

        std::vector<MaterialSlot> mMaterials;
        std::deque<uint32_t>      mFreeMaterialSlots;

        uint32_t       mDefaultAlbedoIdx{~0u};
        uint32_t       mDefaultNormalIdx{~0u};
        uint32_t       mDefaultWhiteIdx{~0u};
        MaterialHandle mDefaultMaterialHandle;

        Texture                      mAssetBrowserDirectoryTexture;
        Texture                      mAssetBrowserFileTexture;
    };
}
