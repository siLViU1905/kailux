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
        registry.allocResources();
        registry.createDefaultTextures(context);
        registry.mDefaultMaterialHandle = *registry.registerMaterial(
            {
                registry.mDefaultAlbedoIdx,
                registry.mDefaultNormalIdx,
                registry.mDefaultWhiteIdx,
                registry.mDefaultWhiteIdx,
                registry.mDefaultWhiteIdx
            });
        registry.createAssetBrowserTextures(context, directoryIconPath, fileIconPath);
        return registry;
    }

    std::optional<TextureHandle> TextureRegistry::registerTexture(Texture &&texture)
    {
        if (mFreeTextureSlots.empty())
            return std::nullopt;

        auto slot = mFreeTextureSlots.front();
        mFreeTextureSlots.pop_front();

        mTextures[slot] = std::move(texture);
        mTextureRefCount[slot] = 1;

        return {{slot}};
    }

    void TextureRegistry::releaseTexture(TextureHandle handle)
    {
        assert(handle.valid() || handle.index < mMaterials.size());
        assert(handle.index != mDefaultAlbedoIdx || handle.index != mDefaultNormalIdx ||
            handle.index != mDefaultWhiteIdx);

        if (mTextureRefCount[handle.index] == 0)
            return;

        if (--mTextureRefCount[handle.index] == 0)
        {
            mTextures[handle.index] = {};
            mFreeTextureSlots.push_back(handle.index);
        }
    }

    const Texture & TextureRegistry::getTexture(TextureHandle handle) const
    {
        assert(handle.valid());
        return *mTextures[handle.index];
    }

    std::optional<MaterialHandle> TextureRegistry::registerMaterial(const MaterialSlot &slot)
    {
        if (mFreeMaterialSlots.empty())
            return std::nullopt;

        auto idx = mFreeMaterialSlots.front();
        mFreeMaterialSlots.pop_front();
        mMaterials[idx] = slot;

        return {{idx}};
    }

    void TextureRegistry::updateMaterial(MaterialHandle handle, const MaterialSlot &slot)
    {
        assert(handle.valid() || handle.index < mMaterials.size());
        mMaterials[handle.index] = slot;
    }

    void TextureRegistry::releaseMaterial(MaterialHandle handle)
    {
        assert(handle.valid() || handle.index < mMaterials.size());
        assert(handle.index != mDefaultMaterialHandle.index);

        const auto& slot = mMaterials[handle.index];
        for (auto textureIdx : {slot.albedoIdx, slot.normalIdx, slot.roughnessIdx,
                             slot.metallicIdx, slot.aoIdx})
            releaseTexture({textureIdx});

        mFreeMaterialSlots.push_back(handle.index);
    }

    const MaterialSlot & TextureRegistry::getMaterial(MaterialHandle handle) const
    {
        assert(handle.valid());
        return mMaterials[handle.index];
    }

    MaterialHandle TextureRegistry::getDefaultMaterialHandle() const
    {
        return mDefaultMaterialHandle;
    }

    TextureHandle TextureRegistry::getDefaultTextureHandle(TextureType type) const
    {
        switch (type)
        {
            case TextureType::Albedo:    return {mDefaultAlbedoIdx};
            case TextureType::Normal:    return {mDefaultNormalIdx};
            default:                     return {mDefaultWhiteIdx};
        }
    }

    std::span<const MaterialSlot> TextureRegistry::viewMaterials() const
    {
        return {mMaterials};
    }

    std::vector<LiveTexture> TextureRegistry::getLiveTexures() const
    {
        std::vector<LiveTexture> liveTextures;
        liveTextures.reserve(mTextures.size());
        for (uint32_t i{}; i < static_cast<uint32_t>(mTextures.size()); ++i)
            if (mTextureRefCount[i] > 0 && mTextures[i].has_value())
                liveTextures.emplace_back(i, std::cref(*mTextures[i]));
        return liveTextures;
    }

    const Texture & TextureRegistry::getAssetBrowserDirectoryIconTexture() const
    {
        return mAssetBrowserDirectoryTexture;
    }

    const Texture & TextureRegistry::getAssetBrowserFileIconTexture() const
    {
        return mAssetBrowserFileTexture;
    }

    AsyncMaterialResult TextureRegistry::createMaterialFromData(const Context &context, const MaterialData &data)
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
                asyncTex.texture.getImage(),
                asyncTex.staging.getBuffer(),
                asyncTex.width,
                asyncTex.height,
                asyncTex.mipLevels
            );
            result.staging.push_back(std::move(asyncTex.staging));

            if (auto handle = registerTexture(std::move(asyncTex.texture)))
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

    void TextureRegistry::allocResources()
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

    void TextureRegistry::createDefaultTextures(const Context &context)
    {
        ImageLoader::ImageData data{};
        data = {
            1,
            1,
            1,
            {191, 191, 191, 255}
        };
        mDefaultAlbedoIdx = registerTexture(TextureAllocator::create_from_image_data(context, data))->index;

        data = {
            1,
            1,
            1,
            {128, 128, 255, 255}
        };
       mDefaultNormalIdx = registerTexture(TextureAllocator::create_from_image_data(context, data))->index;

        data = {
            1,
            1,
            1,
            {255, 255, 255, 255}
        };
        mDefaultWhiteIdx = registerTexture(TextureAllocator::create_from_image_data(context, data))->index;
    }

    void TextureRegistry::createAssetBrowserTextures(const Context &context, std::string_view directoryIconPath,
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
