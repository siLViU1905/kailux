#pragma once
#include <assimp/material.h>

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

    struct AsyncTextureUpload
    {
        TextureHandle handle;
        ImageUpload   upload;
        Buffer        staging;
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
        static constexpr std::array kTextureTypes{
            TextureType::Albedo,
            TextureType::Normal,
            TextureType::Roughness,
            TextureType::Metallic,
            TextureType::AO
        };

        static constexpr uint32_t texture_type_index(TextureType type)
        {
            for (uint32_t i{}; i < kTextureTypes.size(); ++i)
                if (kTextureTypes[i] == type)
                    return i;
            return 0;
        }

        static constexpr std::string_view texture_type_name(TextureType type)
        {
            switch (type)
            {
                case TextureType::Albedo:    return "Albedo";
                case TextureType::Normal:    return "Normal";
                case TextureType::Roughness: return "Roughness";
                case TextureType::Metallic:  return "Metallic";
                case TextureType::AO:        return "AO";
            }
            return "";
        }

        static constexpr image_loader::ColorSpace texture_color_space(TextureType type)
        {
            return type == TextureType::Albedo ? image_loader::ColorSpace::Srgb : image_loader::ColorSpace::Linear;
        }

        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(TextureRegistry)

        static TextureRegistry create(const Context &context,
                                      std::string_view directoryIconPath,
                                      std::string_view fileIconPath
        );

        std::optional<TextureHandle>  RegisterTexture(Texture&& texture);
        bool                          ReleaseTexture(TextureHandle handle);
        void                          AcquireTexture(TextureHandle handle);
        bool                          IsDefaultTexture(TextureHandle handle) const;
        const Texture&                GetTexture(TextureHandle handle) const;
        std::optional<MaterialHandle> RegisterMaterial(const MaterialSlot& slot);
        void                          UpdateMaterial(MaterialHandle handle, const MaterialSlot& slot);
        std::optional<MaterialHandle> CloneMaterial(MaterialHandle source);
        TextureHandle                 UpdateTexture(MaterialHandle handle, TextureType type, TextureHandle texture);
        TextureHandle                 GetMaterialTexture(MaterialHandle handle, TextureType type) const;
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

        std::optional<AsyncTextureUpload> CreateTextureFromData(const Context &context, const image_loader::ImageData& data);

    private:
        void     AllocResources();
        void     CreateDefaultTextures(const Context& context);
        void     CreateAssetBrowserTextures(const Context& context, std::string_view directoryIconPath, std::string_view fileIconPath);

        static uint32_t& slot_index(MaterialSlot& slot, TextureType type);

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
