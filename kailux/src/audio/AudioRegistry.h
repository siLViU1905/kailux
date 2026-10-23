#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec3.hpp>
#include "Core.h"

struct ALCdevice;
struct ALCcontext;
namespace kailux
{
    using AudioClipHandle   = Handle;
    using AudioSourceHandle = Handle;

    struct AudioSourceSettings
    {
        float gain{1.f};
        float pitch{1.f};
        bool  loop{false};
        bool  playOnStart{true};
        bool  spatial{true};

        float referenceDistance{1.f};
        float maxDistance{50.f};
        float rolloff{1.f};
    };

    enum class AudioSourceState : uint8_t
    {
        Initial,
        Playing,
        Paused,
        Stopped
    };

    struct AudioListenerPose
    {
        glm::vec3 position{0.f};
        glm::vec3 velocity{0.f};
        glm::vec3 forward{0.f, 0.f, -1.f};
        glm::vec3 up{0.f, 1.f, 0.f};
    };

    class AudioRegistry
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(AudioRegistry)

        static AudioRegistry create(bool requestHrtf);

        bool             Available() const;
        std::string_view GetDeviceName() const;
        bool             IsHrtfEnabled() const;

        using ClipResult = std::expected<AudioClipHandle, std::string>;
        ClipResult LoadClip(const std::filesystem::path &path, bool mono);

        using SourceResult = std::expected<AudioSourceHandle, std::string>;
        SourceResult CreateSource();
        void         DestroySource(AudioSourceHandle handle);
        bool         IsSourceValid(AudioSourceHandle handle) const;

        void SetSourceClip(AudioSourceHandle source, AudioClipHandle clip);
        void SetSourceSettings(AudioSourceHandle handle, const AudioSourceSettings &settings);
        void SetSourcePosition(AudioSourceHandle handle, const glm::vec3 &position);
        void SetSourceVelocity(AudioSourceHandle handle, const glm::vec3 &velocity);

        void             Play(AudioSourceHandle handle);
        void             Pause(AudioSourceHandle handle);
        void             Stop(AudioSourceHandle handle);
        AudioSourceState GetState(AudioSourceHandle handle) const;

        void  SetListener(const AudioListenerPose &pose);
        void  SetListenerGain(float gain);
        float GetListenerGain() const;

        ~AudioRegistry();

    private:
        void Release();

        uint32_t GetSource(AudioSourceHandle handle) const;

        struct DeviceDeleter
        {
            void operator()(ALCdevice* device) const;
        };
        struct ContextDeleter
        {
            void operator()(ALCcontext *context) const;
        };

        std::unique_ptr<ALCdevice, DeviceDeleter>   mDevice;
        std::unique_ptr<ALCcontext, ContextDeleter> mContext;

        std::string mDeviceName;
        bool        mHrtfEnabled{};

        std::vector<uint32_t>                            mBuffers;
        std::unordered_map<std::string, AudioClipHandle> mClipCache;

        std::vector<uint32_t> mSources;
        std::vector<uint32_t> mFreeSources;
    };
}