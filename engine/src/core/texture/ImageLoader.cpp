#include "ImageLoader.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace kailux
{
    ImageLoader::LoadResult ImageLoader::load_image(std::string_view path)
    {
        int width, height, channels;
        ImageData::Pixel* data = stbi_load(
            path.data(),
            &width,
            &height,
            &channels,
            STBI_rgb_alpha
            );
        if (!data)
            return std::unexpected("Failed to load image from: " + std::string(path));

        auto uWidth = static_cast<uint32_t>(width);
        auto uHeight = static_cast<uint32_t>(height);
        auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(uWidth, uHeight)))) + 1;

        size_t imageSize = uWidth * uHeight * ImageData::s_Channels;
        std::vector pixels(data, data + imageSize);

        stbi_image_free(data);

        return { {uWidth, uHeight, mipLevels, std::move(pixels)} };
    }
}
