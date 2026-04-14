#include "TextureRegistry.h"

#include "TextureAllocator.h"

namespace kailux
{
    TextureRegistry::TextureRegistry() = default;

    TextureRegistry::TextureRegistry(TextureRegistry &&other) noexcept : m_DefaultSet(other.m_DefaultSet),
                                                                         m_TexturePool(std::move(other.m_TexturePool)),
                                                                         m_FreeSlots(std::move(other.m_FreeSlots))
    {
    }

    TextureRegistry &TextureRegistry::operator=(TextureRegistry &&other) noexcept
    {
        if (this != &other)
        {
            m_DefaultSet = other.m_DefaultSet;
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
        return registry;
    }

    Shared<Texture> TextureRegistry::view(TextureHandle handle) const
    {
        assert(handle.valid());
        return m_TexturePool[handle.index];
    }

    TextureHandle TextureRegistry::registerTexture(Shared<Texture> texture)
    {
        auto slot = acquireSlot();
        m_TexturePool[slot] = texture;
        return {slot};
    }

    void TextureRegistry::unregisterTexture(TextureHandle handle)
    {
        if (!handle.valid())
            return;

        m_TexturePool[handle.index].reset();

        m_FreeSlots.push_back(handle.index);
    }

    uint32_t TextureRegistry::acquireSlot()
    {
        if (m_FreeSlots.empty())
            return TextureHandle::s_InvalidIndex;
        auto slot = m_FreeSlots.front();
        m_FreeSlots.pop_front();
        return slot;
    }

    TextureSet TextureRegistry::getDefaultSet() const
    {
        return m_DefaultSet;
    }

    void TextureRegistry::allocResources(uint32_t meshCount)
    {
        m_TexturePool.resize(meshCount * s_TextureTypes);

        for (uint32_t i = 0; i < meshCount * s_TextureTypes; ++i)
            m_FreeSlots.push_back(i);
    }

    void TextureRegistry::createDefaultTextures(const Context &context)
    {
        ImageLoader::ImageData data;
        data = {
            1,
            1,
            1,
            {191, 191, 191, 255}
        };
        auto texture = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));
        m_DefaultSet.albedo = registerTexture(texture);

        data = {
            1,
            1,
            1,
            {128, 128, 255, 255}
        };
        texture = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));
        m_DefaultSet.normal = registerTexture(texture);

        data = {
            1,
            1,
            1,
            {255, 255, 255, 255}
        };
        texture = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));
        m_DefaultSet.roughness = registerTexture(texture);

        data = {
            1,
            1,
            1,
            {0, 0, 0, 255}
        };
        texture = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));
        m_DefaultSet.metallic = registerTexture(texture);

        data = {
            1,
            1,
            1,
            {255, 255, 255, 255}
        };
        texture = create_shared<Texture>(TextureAllocator::create_from_image_data(context, data));
        m_DefaultSet.ao = registerTexture(texture);
    }
}
