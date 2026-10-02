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

        vk::Image      GetImage() const;
        vk::ImageView  GetImageView() const;
        vk::Sampler    GetSampler() const;

    private:
        vk::raii::Image        mImage;
        vk::raii::ImageView    mImageView;
        vk::raii::Sampler      mSampler;
        vk::raii::DeviceMemory mMemory;
    };
}
