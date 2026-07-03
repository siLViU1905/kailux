#include "TextureRegistry.h"

#include "TextureAllocator.h"

namespace kailux
{
    TextureRegistry::TextureRegistry() = default;

    TextureRegistry::TextureRegistry(TextureRegistry &&other) noexcept : mDefaultSet(std::move(other.mDefaultSet)),
                                                                         mDefaultSetHandle(other.mDefaultSetHandle),
                                                                         mTexturePool(std::move(other.mTexturePool)),
                                                                         mFreeSlots(std::move(other.mFreeSlots)),
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
            mDefaultSet = std::move(other.mDefaultSet);
            mDefaultSetHandle = other.mDefaultSetHandle;
            mTexturePool = std::move(other.mTexturePool);
            mFreeSlots = std::move(other.mFreeSlots);
            mAssetBrowserDirectoryTexture = std::move(other.mAssetBrowserDirectoryTexture);
            mAssetBrowserFileTexture = std::move(other.mAssetBrowserFileTexture);
        }
        return *this;
    }

    TextureRegistry TextureRegistry::create(const Context &context,
                                      uint32_t meshCount,
                                      std::string_view directoryIconPath,
                                      std::string_view fileIconPath
                                      )
    {
        TextureRegistry registry;
        registry.allocResources(meshCount);
        registry.createDefaultTextures(context);
        registry.mDefaultSetHandle = registry.registerTextureSet(registry.mDefaultSet);
        registry.createAssetBrowserTextures(context, directoryIconPath, fileIconPath);
        return registry;
    }

    TextureSetHandle TextureRegistry::registerTextureSet(const TextureSet &set)
    {
        auto slot = acquireSlot();
        mTexturePool[slot] = set;
        return {slot};
    }

    const TextureSet &TextureRegistry::view(TextureSetHandle handle) const
    {
        assert(handle.valid());
        return mTexturePool[handle.index];
    }

    void TextureRegistry::unregisterTextureSet(TextureSetHandle handle)
    {
        if (!handle.valid())
            return;
        mTexturePool[handle.index] = mDefaultSet;

        mFreeSlots.push_back(handle.index);
    }

    void TextureRegistry::updateTextureSet(TextureSetHandle handle, const TextureSet &set)
    {
        if (!handle.valid())
            return;
        mTexturePool[handle.index] = set;
    }

    uint32_t TextureRegistry::acquireSlot()
    {
        if (mFreeSlots.empty())
            return TextureSetHandle::kInvalidIndex;
        auto slot = mFreeSlots.front();
        mFreeSlots.pop_front();
        return slot;
    }

    TextureSetHandle TextureRegistry::getDefaultSetHandle() const
    {
        return mDefaultSetHandle;
    }

    const Texture & TextureRegistry::getAssetBrowserDirectoryIconTexture() const
    {
        return mAssetBrowserDirectoryTexture;
    }

    const Texture & TextureRegistry::getAssetBrowserFileIconTexture() const
    {
        return mAssetBrowserFileTexture;
    }

    AsyncMaterialResult TextureRegistry::createSetFromMaterialData(const Context &context, const MaterialData &data) const
    {
        auto checkSize = [](const ImageLoader::ImageData &data)-> bool
        {
            return data.width && data.height;
        };

        AsyncMaterialResult result;
        result.set = mDefaultSet;

        auto process = [&](const auto &imgData, auto &target)
        {
            if (!checkSize(imgData))
                return;

            auto asyncTex = TextureAllocator::create_from_image_data_async(context, imgData);
            target = create_shared<Texture>(std::move(asyncTex.texture));

            result.uploads.emplace_back(
                target->getImage(),
                asyncTex.staging.getBuffer(),
                asyncTex.width,
                asyncTex.height,
                asyncTex.mipLevels
            );
            result.staging.push_back(std::move(asyncTex.staging));
        };

        process(data.albedoData,    result.set.albedo);
        process(data.normalData,    result.set.normal);
        process(data.roughnessData, result.set.roughness);
        process(data.metallicData,  result.set.metallic);
        process(data.aoData,        result.set.ao);

        return result;
    }

    void TextureRegistry::allocResources(uint32_t meshCount)
    {
        mTexturePool.resize(meshCount, mDefaultSet);

        for (uint32_t i = 0; i < meshCount; ++i)
            mFreeSlots.push_back(i);
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
        mDefaultSet.albedo = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));

        data = {
            1,
            1,
            1,
            {128, 128, 255, 255}
        };
        mDefaultSet.normal = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));

        data = {
            1,
            1,
            1,
            {255, 255, 255, 255}
        };
        mDefaultSet.roughness = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));

        data = {
            1,
            1,
            1,
            {255, 255, 255, 255}
        };
        mDefaultSet.metallic = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));

        data = {
            1,
            1,
            1,
            {255, 255, 255, 255}
        };
        mDefaultSet.ao = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));
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
