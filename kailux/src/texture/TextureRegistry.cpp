#include "TextureRegistry.h"

#include "TextureAllocator.h"
#include "Log.h"

namespace kailux
{
    TextureRegistry::TextureRegistry() = default;

    TextureRegistry::TextureRegistry(TextureRegistry &&other) noexcept : mTexturePool(std::move(other.mTexturePool)),
                                                                         mTextures(std::move(other.mTextures)),
                                                                         mTextureRefCount(std::move(other.mTextureRefCount)),
                                                                         mFreeTextureSlots(std::move(other.mFreeTextureSlots)),
                                                                         mMaterials(std::move(other.mMaterials)),
                                                                         mFreeMaterialSlots(std::move(other.mFreeMaterialSlots)),
                                                                         mMaterialGeneration(other.mMaterialGeneration),
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
            mTexturePool = std::move(other.mTexturePool);
            mTextures = std::move(other.mTextures);
            mTextureRefCount = std::move(other.mTextureRefCount);
            mFreeTextureSlots = std::move(other.mFreeTextureSlots);
            mMaterials = std::move(other.mMaterials);
            mFreeMaterialSlots = std::move(other.mFreeMaterialSlots);
            mMaterialGeneration = other.mMaterialGeneration;
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
        registry.mTexturePool = TextureMemoryPool::create(context, details::kTexturePoolSize);
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

    bool TextureRegistry::ReleaseTexture(TextureHandle handle)
    {
        assert(handle.Valid() || handle.index < mMaterials.size());

        if (IsDefaultTexture(handle))
            return false;

        if (mTextureRefCount[handle.index] == 0)
            return false;

        if (--mTextureRefCount[handle.index] == 0)
        {
            const auto poolOffset{mTextures[handle.index]->GetPoolOffset()};
            mTextures[handle.index] = {};
            if (poolOffset)
                mTexturePool.Free(*poolOffset);
            mFreeTextureSlots.push_back(handle.index);
            return true;
        }
        return false;
    }

    void TextureRegistry::AcquireTexture(TextureHandle handle)
    {
        assert(handle.Valid() && handle.index < mTextures.size());

        if (IsDefaultTexture(handle) || mTextureRefCount[handle.index] == 0)
            return;

        ++mTextureRefCount[handle.index];
    }

    bool TextureRegistry::IsDefaultTexture(TextureHandle handle) const
    {
        return handle.index == mDefaultAlbedoIdx ||
               handle.index == mDefaultNormalIdx ||
               handle.index == mDefaultWhiteIdx;
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
        ++mMaterialGeneration;

        return {{idx}};
    }

    void TextureRegistry::UpdateMaterial(MaterialHandle handle, const MaterialSlot &slot)
    {
        assert(handle.Valid() || handle.index < mMaterials.size());
        mMaterials[handle.index] = slot;
        ++mMaterialGeneration;
    }

    std::optional<MaterialHandle> TextureRegistry::CloneMaterial(MaterialHandle source)
    {
        assert(source.Valid() && source.index < mMaterials.size());

        const auto slot{mMaterials[source.index]};
        const auto clone{RegisterMaterial(slot)};
        if (!clone)
            return std::nullopt;

        for (const auto textureIdx: {
                 slot.albedoIdx, slot.normalIdx, slot.roughnessIdx,
                 slot.metallicIdx, slot.aoIdx
             })
            AcquireTexture({textureIdx});

        return clone;
    }

    TextureHandle TextureRegistry::UpdateTexture(MaterialHandle handle, TextureType type, TextureHandle texture)
    {
        assert(handle.Valid() && handle.index < mMaterials.size());
        assert(handle.index != mDefaultMaterialHandle.index && "The default material is shared, clone it first");

        auto& index{slot_index(mMaterials[handle.index], type)};
        const TextureHandle previous{std::exchange(index, texture.index)};
        ++mMaterialGeneration;

        return previous;
    }

    TextureHandle TextureRegistry::GetMaterialTexture(MaterialHandle handle, TextureType type) const
    {
        assert(handle.Valid() && handle.index < mMaterials.size());
        auto slot{mMaterials[handle.index]};
        return {slot_index(slot, type)};
    }

    std::vector<uint32_t> TextureRegistry::ReleaseMaterial(MaterialHandle handle)
    {
        assert(handle.Valid() && handle.index < mMaterials.size());
        if (handle.index == mDefaultMaterialHandle.index)
            return {};

        std::vector<uint32_t> freedTextureSlots;
        const auto& slot = mMaterials[handle.index];
        for (auto textureIdx : {slot.albedoIdx, slot.normalIdx, slot.roughnessIdx,
                             slot.metallicIdx, slot.aoIdx})
            if (ReleaseTexture({textureIdx}))
                freedTextureSlots.push_back(textureIdx);

        mFreeMaterialSlots.push_back(handle.index);
        ++mMaterialGeneration;

        return freedTextureSlots;
    }

    bool TextureRegistry::HasFreeMaterialSlot() const
    {
        return !mFreeMaterialSlots.empty();
    }

    uint64_t TextureRegistry::GetMaterialGeneration() const
    {
        return mMaterialGeneration;
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
        AsyncMaterialResult result;
        result.slot.albedoIdx    = mDefaultAlbedoIdx;
        result.slot.normalIdx    = mDefaultNormalIdx;
        result.slot.roughnessIdx = mDefaultWhiteIdx;
        result.slot.metallicIdx  = mDefaultWhiteIdx;
        result.slot.aoIdx        = mDefaultWhiteIdx;

        auto process = [&](const auto &imgData, auto &slotIndexOut)
        {
            auto texture{CreateTextureFromData(context, imgData)};
            if (!texture)
                return;

            result.uploads.push_back(texture->upload);
            result.staging.push_back(std::move(texture->staging));

            slotIndexOut = texture->handle.index;
            result.handles.push_back(texture->handle);
        };

        process(data.albedoData,    result.slot.albedoIdx);
        process(data.normalData,    result.slot.normalIdx);
        process(data.roughnessData, result.slot.roughnessIdx);
        process(data.metallicData,  result.slot.metallicIdx);
        process(data.aoData,        result.slot.aoIdx);

        return result;
    }

    std::optional<AsyncTextureUpload> TextureRegistry::CreateTextureFromData(const Context &context,
        const image_loader::ImageData &data)
    {
        if (!data.width || !data.height || mFreeTextureSlots.empty())
                return std::nullopt;

        auto asyncTex{texture_allocator::create_from_image_data_async(context, mTexturePool, data)};
        if (!asyncTex)
        {
            log::console.Warning("texture registry: memory pool full, using default texture");
            return std::nullopt;
        }

        ImageUpload upload{
                asyncTex->texture.GetImage(),
                asyncTex->staging.GetBuffer(),
                asyncTex->width,
                asyncTex->height,
                asyncTex->mipLevels
            };

        const auto handle{RegisterTexture(std::move(asyncTex->texture))};
        return AsyncTextureUpload{*handle, upload, std::move(asyncTex->staging)};
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
        image_loader::ImageData data{};
        data = {
            1,
            1,
            1,
            {191, 191, 191, 255}
        };
        mDefaultAlbedoIdx = RegisterTexture(*texture_allocator::create_from_image_data(context, mTexturePool, data))->index;

        data = {
            1,
            1,
            1,
            {128, 128, 255, 255}
        };
       mDefaultNormalIdx = RegisterTexture(*texture_allocator::create_from_image_data(context, mTexturePool, data))->index;

        data = {
            1,
            1,
            1,
            {255, 255, 255, 255}
        };
        mDefaultWhiteIdx = RegisterTexture(*texture_allocator::create_from_image_data(context, mTexturePool, data))->index;
    }

    void TextureRegistry::CreateAssetBrowserTextures(const Context &context, std::string_view directoryIconPath,
                                                     std::string_view fileIconPath)
    {
        auto imgData = image_loader::load_image(directoryIconPath, image_loader::ColorSpace::Srgb);
        if (imgData)
            mAssetBrowserDirectoryTexture = texture_allocator::create_from_image_data(context, *imgData);

        imgData = image_loader::load_image(fileIconPath, image_loader::ColorSpace::Srgb);
        if (imgData)
            mAssetBrowserFileTexture = texture_allocator::create_from_image_data(context, *imgData);
    }

    uint32_t & TextureRegistry::slot_index(MaterialSlot &slot, TextureType type)
    {
        switch (type)
        {
            case TextureType::Albedo: return slot.albedoIdx;
            case TextureType::Normal: return slot.normalIdx;
            case TextureType::Roughness: return slot.roughnessIdx;
            case TextureType::Metallic: return slot.metallicIdx;
            case TextureType::AO: return slot.aoIdx;
        }
        return slot.albedoIdx;
    }
}
