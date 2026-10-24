#include "Texture.h"
#include <utility>

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
                                                 mMemory(std::move(other.mMemory)),
                                                 mPoolOffset(std::exchange(other.mPoolOffset, std::nullopt))
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
            mPoolOffset = std::exchange(other.mPoolOffset, std::nullopt);
        }
        return *this;
    }

    Texture::Texture(vk::raii::Image &&image,
                     vk::raii::DeviceMemory &&memory,
                     vk::raii::ImageView &&view,
                     vk::raii::Sampler &&sampler) : mImage(std::move(image)),
                                                    mImageView(std::move(view)),
                                                    mSampler(std::move(sampler)),
                                                    mMemory(std::move(memory)),
                                                    mPoolOffset({})
    {
    }

    Texture::Texture(vk::raii::Image &&image,
                     vk::DeviceSize poolOffset,
                     vk::raii::ImageView &&view,
                     vk::raii::Sampler &&sampler) : mImage(std::move(image)),
                                                    mImageView(std::move(view)),
                                                    mSampler(std::move(sampler)),
                                                    mMemory({}),
                                                    mPoolOffset(poolOffset)
    {
    }

    vk::Image Texture::GetImage() const
    {
        return *mImage;
    }

    vk::ImageView Texture::GetImageView() const
    {
        return *mImageView;
    }

    vk::Sampler Texture::GetSampler() const
    {
        return *mSampler;
    }

    std::optional<vk::DeviceSize> Texture::GetPoolOffset() const
    {
        return mPoolOffset;
    }
}
