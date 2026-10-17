#pragma once
#include <expected>

namespace kailux::image_loader
{
    enum class ColorSpace
    {
        Srgb,
        Linear
    };

    struct ImageData
    {
        static constexpr uint32_t kChannels{4};

        uint32_t width{};
        uint32_t height{};
        uint32_t mipLevels{};
        using Pixel = uint8_t;
        std::vector<Pixel> pixels;
    };

    using Result = std::expected<ImageData, std::string>;

    Result load_image(const std::filesystem::path &path, ColorSpace space);

    namespace details
    {
        Result cap_image(ImageData &image, ColorSpace space);
    }
}
