#include "TextureRegistry.h"

#include "TextureAllocator.h"

namespace kailux
{
    TextureRegistry::TextureRegistry() = default;

    TextureRegistry::TextureRegistry(TextureRegistry &&other) noexcept : mTextures(std::move(other.mTextures)),
                                                                         mTextureRefCount(std::move(other.mTextureRefCount)),
                                                                         mFreeTextureSlots(std::move(other.mFreeTextureSlots)),
                                                                         mMaterials(std::move(other.mMaterials)),
                                                                         mFreeMaterialSlots(std::move(other.mFreeMaterialSlots)),
                                                                         mDefaultAlbedoIdx(other.mDefaultAlbedoIdx),
                                                                         mDefaultNormalIdx(other.mDefaultNormalIdx),
                                                                         mDefaultWhiteIdx(other.mDefaultWhiteIdx),
                                                                         mDefaultMaterialHandle(other.mDefaultMaterialHandle),
                                                                         mAssetBrowserDirectoryTexture(
                                                                             std::move(
                                                                                 other.mAssetBrowserDirectoryTexture)),
                                                                         mAssetBrowserFileTexture(
                                                                             std::move(other.mAssetBrowserFileTexture))
    {
    }

    TextureRegistry &TextureRegistry::operator=(TextureRegistry &&other) noexcept
    {
        if (this != &other)
        {
            mTextures = std::move(other.mTextures);
            mTextureRefCount = std::move(other.mTextureRefCount);
            mFreeTextureSlots = std::move(other.mFreeTextureSlots);
            mMaterials = std::move(other.mMaterials);
            mFreeMaterialSlots = std::move(other.mFreeMaterialSlots);
            mDefaultAlbedoIdx = other.mDefaultAlbedoIdx;
            mDefaultNormalIdx = other.mDefaultNormalIdx;
            mDefaultWhiteIdx = other.mDefaultWhiteIdx;
            mDefaultMaterialHandle = other.mDefaultMaterialHandle;
            mAssetBrowserDirectoryTexture = std::move(other.mAssetBrowserDirectoryTexture);
            mAssetBrowserFileTexture = std::move(other.mAssetBrowserFileTexture);
        }
        return *this;
    }

    TextureRegistry TextureRegistry::create(const Context &context,
                                            std::string_view directoryIconPath,
                                            std::string_view fileIconPath
    )
    {
        TextureRegistry registry;
        registry.AllocResources();
        registry.CreateDefaultTextures(context);
        registry.mDefaultMaterialHandle = *registry.RegisterMaterial(
            {
                registry.mDefaultAlbedoIdx,
                registry.mDefaultNormalIdx,
                registry.mDefaultWhiteIdx,
                registry.mDefaultWhiteIdx,
                registry.mDefaultWhiteIdx
            });
        registry.CreateAssetBrowserTextures(context, directoryIconPath, fileIconPath);
        return registry;
    }

    std::optional<TextureHandle> TextureRegistry::RegisterTexture(Texture &&texture)
    {
        if (mFreeTextureSlots.empty())
            return std::nullopt;

        auto slot = mFreeTextureSlots.front();
        mFreeTextureSlots.pop_front();

        mTextures[slot] = std::move(texture);
        mTextureRefCount[slot] = 1;

        return {{slot}};
    }

    void TextureRegistry::ReleaseTexture(TextureHandle handle)
    {
        assert(handle.Valid() || handle.index < mMaterials.size());

        if (handle.index == mDefaultAlbedoIdx ||
            handle.index == mDefaultNormalIdx ||
            handle.index == mDefaultWhiteIdx)
            return;

        if (mTextureRefCount[handle.index] == 0)
            return;

        if (--mTextureRefCount[handle.index] == 0)
        {
            mTextures[handle.index] = {};
            mFreeTextureSlots.push_back(handle.index);
        }
    }

    const Texture & TextureRegistry::GetTexture(TextureHandle handle) const
    {
        assert(handle.Valid());
        return *mTextures[handle.index];
    }

    std::optional<MaterialHandle> TextureRegistry::RegisterMaterial(const MaterialSlot &slot)
    {
        if (mFreeMaterialSlots.empty())
            return std::nullopt;

        auto idx = mFreeMaterialSlots.front();
        mFreeMaterialSlots.pop_front();
        mMaterials[idx] = slot;

        return {{idx}};
    }

    void TextureRegistry::UpdateMaterial(MaterialHandle handle, const MaterialSlot &slot)
    {
        assert(handle.Valid() || handle.index < mMaterials.size());
        mMaterials[handle.index] = slot;
    }

    void TextureRegistry::ReleaseMaterial(MaterialHandle handle)
    {
        assert(handle.Valid() && handle.index < mMaterials.size());
        if (handle.index == mDefaultMaterialHandle.index)
            return;

        const auto& slot = mMaterials[handle.index];
        for (auto textureIdx : {slot.albedoIdx, slot.normalIdx, slot.roughnessIdx,
                             slot.metallicIdx, slot.aoIdx})
            ReleaseTexture({textureIdx});

        mFreeMaterialSlots.push_back(handle.index);
    }

    const MaterialSlot & TextureRegistry::GetMaterial(MaterialHandle handle) const
    {
        assert(handle.Valid());
        return mMaterials[handle.index];
    }

    MaterialHandle TextureRegistry::GetDefaultMaterialHandle() const
    {
        return mDefaultMaterialHandle;
    }

    TextureHandle TextureRegistry::GetDefaultTextureHandle(TextureType type) const
    {
        switch (type)
        {
            case TextureType::Albedo:    return {mDefaultAlbedoIdx};
            case TextureType::Normal:    return {mDefaultNormalIdx};
            default:                     return {mDefaultWhiteIdx};
        }
    }

    std::span<const MaterialSlot> TextureRegistry::ViewMaterials() const
    {
        return {mMaterials};
    }

    std::vector<LiveTexture> TextureRegistry::GetLiveTexures() const
    {
        std::vector<LiveTexture> liveTextures;
        liveTextures.reserve(mTextures.size());
        for (uint32_t i{}; i < static_cast<uint32_t>(mTextures.size()); ++i)
            if (mTextureRefCount[i] > 0 && mTextures[i].has_value())
                liveTextures.emplace_back(i, std::cref(*mTextures[i]));
        return liveTextures;
    }

    const Texture & TextureRegistry::GetAssetBrowserDirectoryIconTexture() const
    {
        return mAssetBrowserDirectoryTexture;
    }

    const Texture & TextureRegistry::GetAssetBrowserFileIconTexture() const
    {
        return mAssetBrowserFileTexture;
    }

    AsyncMaterialResult TextureRegistry::CreateMaterialFromData(const Context &context, const MaterialData &data)
    {
        auto checkSize = [](const auto &imgData)-> bool
        {
            return imgData.width && imgData.height;
        };

        AsyncMaterialResult result;
        result.slot.albedoIdx    = mDefaultAlbedoIdx;
        result.slot.normalIdx    = mDefaultNormalIdx;
        result.slot.roughnessIdx = mDefaultWhiteIdx;
        result.slot.metallicIdx  = mDefaultWhiteIdx;
        result.slot.aoIdx        = mDefaultWhiteIdx;

        auto process = [&](const auto &imgData, auto &slotIndexOut)
        {
            if (!checkSize(imgData))
                return;

            auto asyncTex = TextureAllocator::create_from_image_data_async(context, imgData);

            result.uploads.emplace_back(
                asyncTex.texture.GetImage(),
                asyncTex.staging.GetBuffer(),
                asyncTex.width,
                asyncTex.height,
                asyncTex.mipLevels
            );
            result.staging.push_back(std::move(asyncTex.staging));

            if (auto handle = RegisterTexture(std::move(asyncTex.texture)))
            {
                slotIndexOut = handle->index;
                result.handles.push_back(*handle);
            }
        };

        process(data.albedoData,    result.slot.albedoIdx);
        process(data.normalData,    result.slot.normalIdx);
        process(data.roughnessData, result.slot.roughnessIdx);
        process(data.metallicData,  result.slot.metallicIdx);
        process(data.aoData,        result.slot.aoIdx);

        return result;
    }

    void TextureRegistry::AllocResources()
    {
        mTextures.resize(details::kMaxTextures);
        mTextureRefCount.assign(details::kMaxTextures, {});

        for (uint32_t i = 0; i < details::kMaxTextures; ++i)
            mFreeTextureSlots.push_back(i);

        mMaterials.resize(details::kMaxMaterials);

        mFreeMaterialSlots.clear();
        for (uint32_t i = 0; i < details::kMaxMaterials; ++i)
            mFreeMaterialSlots.push_back(i);
    }

    void TextureRegistry::CreateDefaultTextures(const Context &context)
    {
        ImageLoader::ImageData data{};
        data = {
            1,
            1,
            1,
            {191, 191, 191, 255}
        };
        mDefaultAlbedoIdx = RegisterTexture(TextureAllocator::create_from_image_data(context, data))->index;

        data = {
            1,
            1,
            1,
            {128, 128, 255, 255}
        };
       mDefaultNormalIdx = RegisterTexture(TextureAllocator::create_from_image_data(context, data))->index;

        data = {
            1,
            1,
            1,
            {255, 255, 255, 255}
        };
        mDefaultWhiteIdx = RegisterTexture(TextureAllocator::create_from_image_data(context, data))->index;
    }

    void TextureRegistry::CreateAssetBrowserTextures(const Context &context, std::string_view directoryIconPath,
                                                     std::string_view fileIconPath)
    {
        auto imgData = ImageLoader::load_image(directoryIconPath);
        if (imgData)
            mAssetBrowserDirectoryTexture = TextureAllocator::create_from_image_data(context, *imgData);

        imgData = ImageLoader::load_image(fileIconPath);
        if (imgData)
            mAssetBrowserFileTexture = TextureAllocator::create_from_image_data(context, *imgData);
    }
}
