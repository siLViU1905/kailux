#include "ImageLoader.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

#include "core/Shader.h"

namespace kailux
{
    ImageLoader::Result ImageLoader::load_image(std::string_view path, ColorSpace space)
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

        ImageData image;
        image.width = static_cast<uint32_t>(width);
        image.height = static_cast<uint32_t>(height);

        const auto imageSize{image.width * image.height * ImageData::kChannels};
        image.pixels = {data, data + imageSize};
        stbi_image_free(data);

        const auto largest{std::max(image.width, image.height)};
        if (largest > details::kMaxTextureDimension)
            if (auto result{cap_image(image, space)}; !result)
                return std::unexpected(result.error());

        image.mipLevels = static_cast<uint32_t>(
                              std::floor(
                                  std::log2(
                                      std::max(
                                          image.width, image.height
                                      )
                                  )
                              )
                          ) + 1;

        return image;
    }

    ImageLoader::Result ImageLoader::cap_image(ImageData &image, ColorSpace space)
    {
        const auto largest{std::max(image.width, image.height)};
        const auto ratio{static_cast<float>(details::kMaxTextureDimension) / static_cast<float>(largest)};

        const auto newWidth{std::max(1u, static_cast<uint32_t>(static_cast<float>(image.width) * ratio))};
        const auto newHeight{std::max(1u, static_cast<uint32_t>(static_cast<float>(image.height) * ratio))};

        std::vector<ImageData::Pixel> newPixels(newWidth * newHeight * ImageData::kChannels);

        const int ok{
            space == ColorSpace::Srgb
                ? stbir_resize_uint8_srgb(
                    image.pixels.data(),
                    static_cast<int>(image.width),
                    static_cast<int>(image.height),
                    0,
                    newPixels.data(),
                    static_cast<int>(newWidth),
                    static_cast<int>(newHeight),
                    0,
                    ImageData::kChannels,
                    3,
                    STBIR_FLAG_ALPHA_PREMULTIPLIED
                )
                : stbir_resize_uint8(
                    image.pixels.data(),
                    static_cast<int>(image.width),
                    static_cast<int>(image.height),
                    0,
                    newPixels.data(),
                    static_cast<int>(newWidth),
                    static_cast<int>(newHeight),
                    0,
                    ImageData::kChannels
                )
        };

        if (!ok)
            return std::unexpected("Failed to resize image");

        image.width = newWidth;
        image.height = newHeight;
        image.pixels = std::move(newPixels);

        return image;
    }
}
