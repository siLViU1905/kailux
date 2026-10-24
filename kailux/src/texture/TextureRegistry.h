#pragma once
#include <assimp/material.h>
#include <magic_enum/magic_enum.hpp>

#include "ImageLoader.h"
#include "Texture.h"
#include "TextureMemoryPool.h"
#include "Context.h"
#include "Core.h"
#include "TransferManager.h"
#include "buffer/Buffer.h"
#include "components/gpu/MaterialSlot.h"

namespace kailux
{
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

    struct LiveTexture
    {
        uint32_t                              slot{~0u};
        std::reference_wrapper<const Texture> texture;
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

        std::optional<TextureHandle>  RegisterTexture(Texture&& texture);
        bool                          ReleaseTexture(TextureHandle handle);
        const Texture&                GetTexture(TextureHandle handle) const;
        std::optional<MaterialHandle> RegisterMaterial(const MaterialSlot& slot);
        void                          UpdateMaterial(MaterialHandle handle, const MaterialSlot& slot);
        std::vector<uint32_t>         ReleaseMaterial(MaterialHandle handle);
        bool                          HasFreeMaterialSlot() const;
        uint64_t                      GetMaterialGeneration() const;
        const MaterialSlot&           GetMaterial(MaterialHandle handle) const;
        MaterialHandle                GetDefaultMaterialHandle() const;
        TextureHandle                 GetDefaultTextureHandle(TextureType type) const;

        std::span<const MaterialSlot> ViewMaterials() const;

        std::vector<LiveTexture> GetLiveTexures() const;

        const Texture& GetAssetBrowserDirectoryIconTexture() const;
        const Texture& GetAssetBrowserFileIconTexture() const;

        struct MaterialData
        {
            image_loader::ImageData albedoData;
            image_loader::ImageData normalData;
            image_loader::ImageData roughnessData;
            image_loader::ImageData metallicData;
            image_loader::ImageData aoData;
        };

        AsyncMaterialResult CreateMaterialFromData(const Context &context, const MaterialData& data);

    private:
        void     AllocResources();
        void     CreateDefaultTextures(const Context& context);
        void     CreateAssetBrowserTextures(const Context& context, std::string_view directoryIconPath, std::string_view fileIconPath);

        TextureMemoryPool                    mTexturePool;
        std::vector<std::optional<Texture>>  mTextures;
        std::vector<uint32_t>                mTextureRefCount;
        std::deque<uint32_t>                 mFreeTextureSlots;

        std::vector<MaterialSlot> mMaterials;
        std::deque<uint32_t>      mFreeMaterialSlots;
        uint64_t                  mMaterialGeneration{};

        uint32_t       mDefaultAlbedoIdx{~0u};
        uint32_t       mDefaultNormalIdx{~0u};
        uint32_t       mDefaultWhiteIdx{~0u};
        MaterialHandle mDefaultMaterialHandle;

        Texture                      mAssetBrowserDirectoryTexture;
        Texture                      mAssetBrowserFileTexture;
    };
}
