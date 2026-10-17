#pragma once
#include "ImageLoader.h"
#include "Texture.h"
#include "buffer/Buffer.h"

namespace kailux
{
    class Context;
    
    struct AsyncTextureResult
    {
        Texture        texture;
        Buffer         staging;
        uint32_t       width{};
        uint32_t       height{};
        uint32_t       mipLevels{};
    };
    
    namespace texture_allocator
    {
            Texture create_from_image_data(
                const Context &context,
                const ImageLoader::ImageData &data
            );

            AsyncTextureResult create_from_image_data_async(
                const Context &context,
                const ImageLoader::ImageData &data
            );

            Texture create_cubemap(
                const Context &context,
                const std::array<ImageLoader::ImageData, 6> &faces
            );

            Texture create_empty(
                const Context &context,
                uint32_t width,
                uint32_t height,
                vk::Format format,
                vk::ImageUsageFlags usage,
                vk::ImageAspectFlags aspect,
                vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1
            );

            Texture create_cubemap_with_mips(
                const Context& context,
                std::span<const std::array<ImageLoader::ImageData, 6>> mips
                );

            void record_texture_copy(
                vk::CommandBuffer cmd,
                vk::Image image,
                vk::Buffer staging,
                uint32_t width,
                uint32_t height,
                uint32_t mipLevels
            );

            void record_texture_mipmaps(
                vk::CommandBuffer cmd,
                vk::Image image,
                uint32_t width,
                uint32_t height,
                uint32_t mipLevels
            );
        
        namespace details
        {
            Texture alloc(
                const Context &context,
                uint32_t width,
                uint32_t height,
                uint32_t mipLevels,
                vk::Format format,
                vk::ImageUsageFlags usage,
                vk::ImageAspectFlags aspect,
                vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1
            );

            void transition_layout(
                vk::CommandBuffer cmd,
                vk::Image image,
                uint32_t mipLevels,
                uint32_t layerCount,
                vk::ImageLayout oldLayout, vk::ImageLayout newLayout
            );

            void generate_mipmaps(
                vk::CommandBuffer cmd,
                vk::Image image,
                uint32_t width,
                uint32_t height,
                uint32_t mipLevels
            );

            void generate_mipmaps_cubemap(
                vk::CommandBuffer cmd,
                vk::Image image,
                uint32_t width,
                uint32_t height,
                uint32_t mipLevels,
                uint32_t face
            );
        }
    }
}
