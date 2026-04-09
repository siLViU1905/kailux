#pragma once
#include "ImageLoader.h"
#include "Texture.h"
#include "core/Context.h"

namespace kailux
{
    class TextureAllocator
    {
    public:
        static Texture create_from_image_data(
            const Context &context,
            const ImageLoader::ImageData &data
        );

        static Texture create_cubemap(
            const Context &context,
            const std::array<ImageLoader::ImageData, 6> &faces
        );

    private:
        static Texture alloc(
            const Context &context,
            uint32_t width,
            uint32_t height,
            uint32_t mipLevels,
            vk::Format format,
            vk::ImageUsageFlags usage,
            vk::ImageAspectFlags aspect
        );

        static void transition_layout(
            vk::CommandBuffer cmd,
            vk::Image image,
            uint32_t mipLevels,
            uint32_t layerCount,
            vk::ImageLayout oldLayout, vk::ImageLayout newLayout
        );

        static void generate_mipmaps(
            vk::CommandBuffer cmd,
            vk::Image image,
            uint32_t width,
            uint32_t height,
            uint32_t mipLevels
        );
        static void generate_mipmaps_cubemap(
            vk::CommandBuffer cmd,
            vk::Image image,
            uint32_t width,
            uint32_t height,
            uint32_t mipLevels,
            uint32_t face
        );
    };
}
