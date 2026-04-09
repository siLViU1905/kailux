#include "TextureAllocator.h"

#include "core/buffer/BufferAllocator.h"
#include "core/command/OneTimeCommand.h"

namespace kailux
{
    Texture TextureAllocator::create_from_image_data(const Context &context, const ImageLoader::ImageData &data)
    {
        vk::DeviceSize imageSize = data.pixels.size() * sizeof(ImageLoader::ImageData::Pixel);
        auto stagingBuffer = BufferAllocator::alloc_staging(context, imageSize);
        stagingBuffer.upload(data.pixels.data(), imageSize);

        auto texture = alloc(
            context,
            data.width,
            data.height,
            data.mipLevels,
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eTransferDst,
            vk::ImageAspectFlagBits::eColor
        );

        auto otc = OneTimeCommand::create(context);
        transition_layout(
            otc.getCommandBuffer(),
            texture.getImage(),
            data.mipLevels,
            1,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal
        );

        vk::BufferImageCopy region{};
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = vk::Extent3D(data.width, data.height, 1);

        otc.getCommandBuffer().copyBufferToImage(
            stagingBuffer.getBuffer(),
            texture.getImage(),
            vk::ImageLayout::eTransferDstOptimal,
            region
        );

        generate_mipmaps(
            otc.getCommandBuffer(),
            texture.getImage(),
            data.width,
            data.height,
            data.mipLevels
        );
        otc.submit(context);

        return texture;
    }

    Texture TextureAllocator::create_cubemap(
        const Context &context,
        const std::array<ImageLoader::ImageData, 6> &faces
    )
    {
        uint32_t width = faces[0].width;
        uint32_t height = faces[0].height;
        vk::DeviceSize layerSize = faces[0].pixels.size() * sizeof(ImageLoader::ImageData::Pixel);
        vk::DeviceSize totalSize = layerSize * 6;

        auto stagingBuffer = BufferAllocator::alloc_staging(context, totalSize);

        for (uint32_t i = 0; i < 6; ++i)
            stagingBuffer.upload(faces[i].pixels.data(), layerSize, i * layerSize);

        vk::ImageCreateInfo imageInfo{};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.format = vk::Format::eR8G8B8A8Unorm;
        imageInfo.extent = vk::Extent3D(width, height, 1);
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;
        imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
        imageInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
        imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;

        vk::raii::Image image(context.m_Device, imageInfo);

        auto memRequirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{
            memRequirements.size,
            context.findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
        };
        vk::raii::DeviceMemory memory(context.m_Device, allocInfo);
        image.bindMemory(*memory, 0);

        auto otc = OneTimeCommand::create(context);
        transition_layout(
            otc.getCommandBuffer(),
            *image,
            1,
            6,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal
        );

        vk::BufferImageCopy region{};
        region.bufferOffset = 0;
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 6;
        region.imageExtent = imageInfo.extent;

        otc.getCommandBuffer().copyBufferToImage(stagingBuffer.getBuffer(), *image,
                                                 vk::ImageLayout::eTransferDstOptimal, region);
        transition_layout(
            otc.getCommandBuffer(),
            *image,
            1,
            6,
            vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal
        );
        otc.submit(context);

        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = *image;
        viewInfo.viewType = vk::ImageViewType::eCube;
        viewInfo.format = imageInfo.format;
        viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;

        vk::raii::ImageView imageView(context.m_Device, viewInfo);

        vk::SamplerCreateInfo samplerInfo{};
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

        vk::raii::Sampler sampler(context.m_Device, samplerInfo);

        return {
            std::move(image),
            std::move(memory),
            std::move(imageView),
            std::move(sampler)
        };
    }

    Texture TextureAllocator::alloc(
        const Context &context,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        vk::Format format,
        vk::ImageUsageFlags usage,
        vk::ImageAspectFlags aspect)
    {
        vk::ImageCreateInfo imageInfo{};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.format = format;
        imageInfo.extent = vk::Extent3D(width, height, 1);
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.usage = usage;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;

        vk::raii::Image image(context.m_Device, imageInfo);

        auto memRequirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = context.findMemoryType(
            memRequirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        vk::raii::DeviceMemory memory(context.m_Device, allocInfo);
        image.bindMemory(*memory, 0);

        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = *image;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspect;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        vk::raii::ImageView imageView(context.m_Device, viewInfo);

        vk::SamplerCreateInfo samplerInfo{};
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
        samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
        samplerInfo.unnormalizedCoordinates = vk::False;
        samplerInfo.compareEnable = vk::False;
        samplerInfo.compareOp = vk::CompareOp::eAlways;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.mipLodBias = 0.f;
        samplerInfo.minLod = 0.f;
        samplerInfo.maxLod = static_cast<float>(mipLevels);

        vk::raii::Sampler sampler(context.m_Device, samplerInfo);

        return {
            std::move(image),
            std::move(memory),
            std::move(imageView),
            std::move(sampler)
        };
    }

    void TextureAllocator::transition_layout(
        vk::CommandBuffer cmd,
        vk::Image image,
        uint32_t mipLevels,
        uint32_t layerCount,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout
    )
    {
        vk::ImageMemoryBarrier barrier{};
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layerCount;

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;

        if (oldLayout == vk::ImageLayout::eUndefined &&
            newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eNone;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
                   newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        } else
            throw std::runtime_error("Unsupported layout transition");

        cmd.pipelineBarrier(
            sourceStage,
            destinationStage,
            {},
            nullptr,
            nullptr,
            barrier
        );
    }

    void TextureAllocator::generate_mipmaps(
        vk::CommandBuffer cmd,
        vk::Image image,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels
    )
    {
        vk::ImageMemoryBarrier barrier{};
        barrier.image = image;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        auto mipWidth = static_cast<int32_t>(width);
        auto mipHeight = static_cast<int32_t>(height);

        for (uint32_t i = 1; i < mipLevels; i++)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

            cmd.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eTransfer,
                {},
                nullptr,
                nullptr,
                barrier
            );

            vk::ImageBlit blit{};
            blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
            blit.srcOffsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
            blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;

            blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
            blit.dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);
            blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            cmd.blitImage(
                image,
                vk::ImageLayout::eTransferSrcOptimal,
                image,
                vk::ImageLayout::eTransferDstOptimal,
                blit,
                vk::Filter::eLinear
            );

            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            cmd.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eFragmentShader,
                {},
                nullptr,
                nullptr,
                barrier
            );

            if (mipWidth > 1)
                mipWidth /= 2;
            if (mipHeight > 1)
                mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {},
            nullptr,
            nullptr,
            barrier
        );
    }
}
