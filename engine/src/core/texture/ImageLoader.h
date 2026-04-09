#pragma once
#include <expected>

namespace kailux
{
    class ImageLoader
    {
    public:
        struct ImageData
        {
            static constexpr uint32_t s_Channels = 4;

            uint32_t           width{};
            uint32_t           height{};
            uint32_t           mipLevels{};
            using              Pixel = uint8_t;
            std::vector<Pixel> pixels;
        };

        using LoadResult = std::expected<ImageData, std::string>;
        static LoadResult load_image(std::string_view path);
    };
}
