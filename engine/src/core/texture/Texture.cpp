#include "Texture.h"

namespace kailux
{
    Texture::Texture() : mImage({}),
                         mImageView({}),
                         mSampler({}),
                         mMemory({})
    {
    }

    Texture::Texture(Texture &&other) noexcept : mImage(std::move(other.mImage)),
                                                 mImageView(std::move(other.mImageView)),
                                                 mSampler(std::move(other.mSampler)),
                                                 mMemory(std::move(other.mMemory))
    {
    }

    Texture &Texture::operator=(Texture &&other) noexcept
    {
        if (this != &other)
        {
            mImage = std::move(other.mImage);
            mImageView = std::move(other.mImageView);
            mSampler = std::move(other.mSampler);
            mMemory = std::move(other.mMemory);
        }
        return *this;
    }

    Texture::Texture(vk::raii::Image &&image,
                     vk::raii::DeviceMemory &&memory,
                     vk::raii::ImageView &&view,
                     vk::raii::Sampler &&sampler) : mImage(std::move(image)),
                                                    mImageView(std::move(view)),
                                                    mSampler(std::move(sampler)),
                                                    mMemory(std::move(memory))
    {
    }

    vk::Image Texture::getImage() const
    {
        return *mImage;
    }

    vk::ImageView Texture::getImageView() const
    {
        return *mImageView;
    }

    vk::Sampler Texture::getSampler() const
    {
        return *mSampler;
    }
}
