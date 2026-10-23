#include "AudioLoader.h"

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>
#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>
#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>
#include <stb_vorbis.c>
#undef L
#undef C
#undef R

#include <algorithm>
#include <cctype>
#include <format>

namespace kailux::audio_loader
{
    namespace details
    {
        static std::string get_extension(const std::filesystem::path &path)
        {
            auto extension{path.extension().string()};
            std::ranges::transform(extension, extension.begin(), [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
            return extension;
        }

        template<typename Free>
        static Result take(
            int16_t *frames,
            uint64_t frameCount,
            uint32_t channels,
            uint32_t sampleRate,
            const std::filesystem::path &path,
            Free &&free
        )
        {
            if (!frames)
                return std::unexpected(std::format("Failed to decode audio file '{}'", path.string()));

            AudioData data;
            data.channels = channels;
            data.sampleRate = sampleRate;
            data.samples.assign(frames, frames + frameCount * channels);
            free(frames);

            if (data.channels == 0 || data.channels > 2)
                return std::unexpected(std::format(
                    "Audio file '{}' has {} channels, only mono and stereo are supported", path.string(), channels));

            if (data.samples.empty())
                return std::unexpected(std::format("Audio file '{}' is empty", path.string()));

            return data;
        }
    }

    Result load(const std::filesystem::path &path)
    {
        if (!std::filesystem::exists(path))
            return std::unexpected(std::format("Audio file '{}' does not exist", path.string()));

        const auto extension{details::get_extension(path)};
        const auto file{path.string()};

        if (extension == ".wav")
        {
            unsigned int channels{}, sampleRate{};
            drwav_uint64 frameCount{};
            auto* frames{
                drwav_open_file_and_read_pcm_frames_s16(
                    file.c_str(),
                    &channels,
                    &sampleRate,
                    &frameCount,
                    nullptr
                    )
            };
            return details::take(
                frames,
                frameCount,
                channels,
                sampleRate,
                path,
                [](void *ptr)
                {
                    drwav_free(ptr, nullptr);
                }
            );
        }
        if (extension == ".mp3")
        {
            drmp3_config config{};
            drmp3_uint64 frameCount{};
            auto* frames{
                drmp3_open_file_and_read_pcm_frames_s16(
                    file.c_str(),
                    &config,
                    &frameCount,
                    nullptr
                    )
            };
            return details::take(
                frames,
                frameCount,
                config.channels,
                config.sampleRate,
                path,
                [](void *ptr)
                {
                    drmp3_free(ptr, nullptr);
                }
            );
        }
        if (extension == ".flac")
        {
            unsigned int channels{}, sampleRate{};
            drflac_uint64 frameCount{};
            auto* frames{
                drflac_open_file_and_read_pcm_frames_s16(
                    file.c_str(),
                    &channels,
                    &sampleRate,
                    &frameCount,
                    nullptr
                    )
            };
            return details::take(
                frames,
                frameCount,
                channels,
                sampleRate,
                path,
                [](void *ptr)
                {
                    drflac_free(ptr, nullptr);
                }
            );
        }
        if (extension == ".ogg")
        {
            int channels{}, sampleRate{};
            short* frames{};
            const int frameCount{
                stb_vorbis_decode_filename(
                    file.c_str(),
                    &channels,
                    &sampleRate,
                    &frames
                )
            };
            return details::take(
                frameCount < 0 ? nullptr : frames,
                static_cast<uint64_t>(std::max(frameCount, 0)),
                static_cast<uint32_t>(channels),
                static_cast<uint32_t>(sampleRate),
                path,
                [](void *ptr)
                {
                    std::free(ptr);
                }
            );
        }

        return std::unexpected(std::format("Audio format '{}' is not supported (use .wav, .ogg, .mp3 or .flac)", extension));
    }

    bool is_supported(const std::filesystem::path &path)
    {
        const auto extension{details::get_extension(path)};
        return extension == ".wav" || extension == ".ogg" || extension == ".mp3" || extension == ".flac";
    }

    AudioData to_mono(const AudioData &data)
    {
        if (data.channels == 1)
            return data;

        AudioData mono;
        mono.channels   = 1;
        mono.sampleRate = data.sampleRate;
        mono.samples.resize(data.samples.size() / data.channels);

        for (size_t frame{}; frame < mono.samples.size(); ++frame)
        {
            int32_t sum{};
            for (uint32_t channel{}; channel < data.channels; ++channel)
                sum += data.samples[frame * data.channels + channel];

            mono.samples[frame] = static_cast<AudioData::Sample>(sum / static_cast<int32_t>(data.channels));
        }
        return mono;
    }
}
