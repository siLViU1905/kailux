#pragma once
#include "core/Core.h"

namespace kailux
{
    class Texture
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Texture)
        Texture(
            vk::raii::Image &&image,
            vk::raii::DeviceMemory &&memory,
            vk::raii::ImageView &&view,
            vk::raii::Sampler &&sampler
        );

        vk::Image      getImage() const;
        vk::ImageView  getImageView() const;
        vk::Sampler    getSampler() const;

    private:
        vk::raii::Image        m_Image;
        vk::raii::ImageView    m_ImageView;
        vk::raii::Sampler      m_Sampler;
        vk::raii::DeviceMemory m_Memory;
    };
}
