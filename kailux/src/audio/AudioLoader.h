#pragma once
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace kailux::audio_loader
{
    struct AudioData
    {
        using Sample = int16_t;

        std::vector<Sample> samples;
        uint32_t            channels{};
        uint32_t            sampleRate{};
    };

    using Result = std::expected<AudioData, std::string>;
    Result load(const std::filesystem::path& path);

    bool is_supported(const std::filesystem::path &path);

    AudioData to_mono(const AudioData& data);
}