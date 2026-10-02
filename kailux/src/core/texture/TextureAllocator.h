#pragma once
#include "ImageLoader.h"
#include "Texture.h"
#include "core/Context.h"
#include "core/buffer/Buffer.h"

namespace kailux
{
    struct AsyncTextureResult
    {
        Texture        texture;
        Buffer         staging;
        uint32_t       width{};
        uint32_t       height{};
        uint32_t       mipLevels{};
    };

    class TextureAllocator
    {
    public:
        static Texture create_from_image_data(
            const Context &context,
            const ImageLoader::ImageData &data
        );

        static AsyncTextureResult create_from_image_data_async(
            const Context &context,
            const ImageLoader::ImageData &data
        );

        static Texture create_cubemap(
            const Context &context,
            const std::array<ImageLoader::ImageData, 6> &faces
        );

        static Texture create_empty(
            const Context &context,
            uint32_t width,
            uint32_t height,
            vk::Format format,
            vk::ImageUsageFlags usage,
            vk::ImageAspectFlags aspect,
            vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1
        );

        static Texture create_cubemap_with_mips(
            const Context& context,
            std::span<const std::array<ImageLoader::ImageData, 6>> mips
            );

        static void record_texture_copy(
            vk::CommandBuffer cmd,
            vk::Image image,
            vk::Buffer staging,
            uint32_t width,
            uint32_t height,
            uint32_t mipLevels
        );

        static void record_texture_mipmaps(
            vk::CommandBuffer cmd,
            vk::Image image,
            uint32_t width,
            uint32_t height,
            uint32_t mipLevels
        );

    private:
        static Texture alloc(
            const Context &context,
            uint32_t width,
            uint32_t height,
            uint32_t mipLevels,
            vk::Format format,
            vk::ImageUsageFlags usage,
            vk::ImageAspectFlags aspect,
            vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1
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
