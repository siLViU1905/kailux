#include "TextureRegistry.h"

#include "TextureAllocator.h"

namespace kailux
{
    TextureRegistry::TextureRegistry() = default;

    TextureRegistry::TextureRegistry(TextureRegistry &&other) noexcept : m_DefaultSet(std::move(other.m_DefaultSet)),
                                                                         m_DefaultSetHandle(other.m_DefaultSetHandle),
                                                                         m_TexturePool(std::move(other.m_TexturePool)),
                                                                         m_FreeSlots(std::move(other.m_FreeSlots))
    {
    }

    TextureRegistry &TextureRegistry::operator=(TextureRegistry &&other) noexcept
    {
        if (this != &other)
        {
            m_DefaultSet = std::move(other.m_DefaultSet);
            m_DefaultSetHandle = other.m_DefaultSetHandle;
            m_TexturePool = std::move(other.m_TexturePool);
            m_FreeSlots = std::move(other.m_FreeSlots);
        }
        return *this;
    }

    TextureRegistry TextureRegistry::create(const Context &context, uint32_t meshCount)
    {
        TextureRegistry registry;
        registry.allocResources(meshCount);
        registry.createDefaultTextures(context);
        registry.m_DefaultSetHandle = registry.registerTextureSet(registry.m_DefaultSet);
        return registry;
    }

    TextureSetHandle TextureRegistry::registerTextureSet(const TextureSet &set)
    {
        auto slot = acquireSlot();
        m_TexturePool[slot] = set;
        return {slot};
    }

    const TextureSet &TextureRegistry::view(TextureSetHandle handle) const
    {
        assert(handle.valid());
        return m_TexturePool[handle.index];
    }

    void TextureRegistry::unregisterTextureSet(TextureSetHandle handle)
    {
        if (!handle.valid())
            return;
        m_TexturePool[handle.index] = m_DefaultSet;

        m_FreeSlots.push_back(handle.index);
    }

    void TextureRegistry::updateTextureSet(TextureSetHandle handle, const TextureSet &set)
    {
        if (!handle.valid())
            return;
        m_TexturePool[handle.index] = set;
    }

    uint32_t TextureRegistry::acquireSlot()
    {
        if (m_FreeSlots.empty())
            return TextureSetHandle::s_InvalidIndex;
        auto slot = m_FreeSlots.front();
        m_FreeSlots.pop_front();
        return slot;
    }

    TextureSetHandle TextureRegistry::getDefaultSetHandle() const
    {
        return m_DefaultSetHandle;
    }

    void TextureRegistry::allocResources(uint32_t meshCount)
    {
        m_TexturePool.resize(meshCount, m_DefaultSet);

        for (uint32_t i = 0; i < meshCount; ++i)
            m_FreeSlots.push_back(i);
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
        m_DefaultSet.albedo = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));

        data = {
            1,
            1,
            1,
            {128, 128, 255, 255}
        };
        m_DefaultSet.normal = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));

        data = {
            1,
            1,
            1,
            {255, 255, 255, 255}
        };
        m_DefaultSet.roughness = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));

        data = {
            1,
            1,
            1,
            {0, 0, 0, 255}
        };
        m_DefaultSet.metallic = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));

        data = {
            1,
            1,
            1,
            {255, 255, 255, 255}
        };
        m_DefaultSet.ao = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));
    }
}
