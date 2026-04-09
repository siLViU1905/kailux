#include "Texture.h"

namespace kailux
{
    Texture::Texture() : m_Image({}),
                         m_ImageView({}),
                         m_Sampler({}),
                         m_Memory({})
    {
    }

    Texture::Texture(Texture &&other) noexcept : m_Image(std::move(other.m_Image)),
                                                 m_ImageView(std::move(other.m_ImageView)),
                                                 m_Sampler(std::move(other.m_Sampler)),
                                                 m_Memory(std::move(other.m_Memory))
    {
    }

    Texture &Texture::operator=(Texture &&other) noexcept
    {
        if (this != &other)
        {
            m_Image = std::move(other.m_Image);
            m_ImageView = std::move(other.m_ImageView);
            m_Sampler = std::move(other.m_Sampler);
            m_Memory = std::move(other.m_Memory);
        }
        return *this;
    }

    Texture::Texture(vk::raii::Image &&image,
                     vk::raii::DeviceMemory &&memory,
                     vk::raii::ImageView &&view,
                     vk::raii::Sampler &&sampler) : m_Image(std::move(image)),
                                                    m_ImageView(std::move(view)),
                                                    m_Sampler(std::move(sampler)),
                                                    m_Memory(std::move(memory))
    {
    }

    vk::Image Texture::getImage() const
    {
        return *m_Image;
    }

    vk::ImageView Texture::getImageView() const
    {
        return *m_ImageView;
    }

    vk::Sampler Texture::getSampler() const
    {
        return *m_Sampler;
    }
}
