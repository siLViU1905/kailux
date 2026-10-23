#include "AudioRegistry.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include "AudioLoader.h"
#include "Log.h"

#include <algorithm>
#include <array>
#include <utility>

namespace kailux
{
    namespace details
    {
        static std::string_view describe_al_error(ALenum error)
        {
            switch (error)
            {
                case AL_INVALID_NAME:
                    return "invalid name";
                case AL_INVALID_ENUM:
                    return "invalid enum";
                case AL_INVALID_VALUE:
                    return "invalid value";
                case AL_INVALID_OPERATION:
                    return "invalid operation";
                case AL_OUT_OF_MEMORY:
                    return "out of memory";
                default:
                    return "unknown error";
            }
        }

        static ALenum get_format(const audio_loader::AudioData &data)
        {
            return data.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
        }
    }

    AudioRegistry::AudioRegistry() = default;

    AudioRegistry::AudioRegistry(AudioRegistry &&other) noexcept : mDevice(std::move(other.mDevice)),
                                                                   mContext(std::move(other.mContext)),
                                                                   mDeviceName(std::move(other.mDeviceName)),
                                                                   mHrtfEnabled(other.mHrtfEnabled),
                                                                   mBuffers(std::exchange(other.mBuffers, {})),
                                                                   mClipCache(std::exchange(other.mClipCache, {})),
                                                                   mSources(std::exchange(other.mSources, {})),
                                                                   mFreeSources(std::exchange(other.mFreeSources, {}))
    {
    }

    AudioRegistry & AudioRegistry::operator=(AudioRegistry &&other) noexcept
    {
        if (this != &other)
        {
            Release();

            mDevice      = std::move(other.mDevice);
            mContext     = std::move(other.mContext);
            mDeviceName  = std::move(other.mDeviceName);
            mHrtfEnabled = other.mHrtfEnabled;
            mBuffers     = std::exchange(other.mBuffers, {});
            mClipCache   = std::exchange(other.mClipCache, {});
            mSources     = std::exchange(other.mSources, {});
            mFreeSources = std::exchange(other.mFreeSources, {});
        }
        return *this;
    }

    AudioRegistry AudioRegistry::create(bool requestHrtf)
    {
        AudioRegistry registry;

        registry.mDevice.reset(alcOpenDevice(nullptr));
        if (!registry.mDevice)
        {
            log::file.Warning("OpenAL: no audio device could be opened, audio is disabled");
            return registry;
        }

        std::vector<ALCint> attributes;
        if (alcIsExtensionPresent(registry.mDevice.get(), "ALC_SOFT_HRTF"))
        {
            attributes.emplace_back(ALC_HRTF_SOFT);
            attributes.emplace_back(requestHrtf ? ALC_TRUE : ALC_DONT_CARE_SOFT);
        }
        attributes.emplace_back(0);

        registry.mContext.reset(alcCreateContext(registry.mDevice.get(), attributes.data()));
        if (!registry.mContext || alcMakeContextCurrent(registry.mContext.get()) == ALC_FALSE)
        {
            log::file.Warning("OpenAL: could not create an audio context, audio is disabled");
            registry.mContext.reset();
            registry.mDevice.reset();
            return registry;
        }

        const ALCchar *name{nullptr};
        if (alcIsExtensionPresent(registry.mDevice.get(), "ALC_ENUMERATE_ALL_EXT"))
            name = alcGetString(registry.mDevice.get(), ALC_ALL_DEVICES_SPECIFIER);
        if (!name)
            name = alcGetString(registry.mDevice.get(), ALC_DEVICE_SPECIFIER);
        registry.mDeviceName = name ? name : "unknown";

        if (alcIsExtensionPresent(registry.mDevice.get(), "ALC_SOFT_HRTF"))
        {
            ALCint hrtf{};
            alcGetIntegerv(registry.mDevice.get(), ALC_HRTF_SOFT, 1, &hrtf);
            registry.mHrtfEnabled = hrtf == ALC_TRUE;
        }

        alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

        log::file.Info("OpenAL: '{}' ({}), HRTF {}",
                       registry.mDeviceName,
                       reinterpret_cast<const char *>(alGetString(AL_RENDERER)),
                       registry.mHrtfEnabled ? "on" : "off");
        return registry;
    }

    bool AudioRegistry::Available() const
    {
        return mContext != nullptr;
    }

    std::string_view AudioRegistry::GetDeviceName() const
    {
        return mDeviceName;
    }

    bool AudioRegistry::IsHrtfEnabled() const
    {
        return mHrtfEnabled;
    }

    AudioRegistry::ClipResult AudioRegistry::LoadClip(const std::filesystem::path &path, bool mono)
    {
        if (!Available())
            return std::unexpected("Audio is disabled: no audio device");

        auto key{path.lexically_normal().generic_string()};
        key += mono ? "#mono" : "#source";
        if (const auto it{mClipCache.find(key)}; it != mClipCache.end())
            return it->second;

        auto data{audio_loader::load(path)};
        if (!data)
            return std::unexpected(data.error());
        if (mono)
            *data = audio_loader::to_mono(*data);

        alGetError();
        ALuint buffer{};
        alGenBuffers(1, &buffer);
        alBufferData(
            buffer,
            details::get_format(*data),
            data->samples.data(),
            static_cast<ALsizei>(data->samples.size() * sizeof(audio_loader::AudioData::Sample)),
            static_cast<ALsizei>(data->sampleRate)
        );
        if (const auto error{alGetError()}; error != AL_NO_ERROR)
        {
            alDeleteBuffers(1, &buffer);
            return std::unexpected(std::format("OpenAL could not create a buffer for '{}': {}",
                                               path.string(), details::describe_al_error(error)));
        }

        const AudioClipHandle handle{static_cast<uint32_t>(mBuffers.size())};
        mBuffers.push_back(buffer);
        mClipCache.emplace(std::move(key), handle);
        return handle;
    }

    AudioRegistry::SourceResult AudioRegistry::CreateSource()
    {
        if (!Available())
            return std::unexpected("Audio is disabled: no audio device");

        alGetError();
        ALuint source{};
        alGenSources(1, &source);
        if (const auto error{alGetError()}; error != AL_NO_ERROR)
            return std::unexpected(std::format(
                "OpenAL could not create a source : {}",
                details::describe_al_error(error)
                ));

        if (!mFreeSources.empty())
        {
            const AudioSourceHandle handle{mFreeSources.back()};
            mFreeSources.pop_back();
            mSources[handle.index] = source;
            return handle;
        }

        const AudioSourceHandle handle{static_cast<uint32_t>(mSources.size())};
        mSources.push_back(source);
        return handle;
    }

    void AudioRegistry::DestroySource(AudioSourceHandle handle)
    {
        const auto source{GetSource(handle)};
        if (source == 0)
            return;

        alSourceStop(source);
        alDeleteSources(1, &source);
        mSources[handle.index] = 0;
        mFreeSources.push_back(handle.index);
    }

    bool AudioRegistry::IsSourceValid(AudioSourceHandle handle) const
    {
        return GetSource(handle) != 0;
    }

    void AudioRegistry::SetSourceClip(AudioSourceHandle source, AudioClipHandle clip)
    {
        const auto src{GetSource(source)};
        if (src == 0)
            return;

        const bool hasClip{clip.Valid() && clip.index < mBuffers.size()};
        alSourceStop(src);
        alSourcei(
            src,
            AL_BUFFER,
            hasClip ? static_cast<ALint>(mBuffers[clip.index]) : 0
            );
    }

    void AudioRegistry::SetSourceSettings(AudioSourceHandle handle, const AudioSourceSettings &settings)
    {
        const auto source{GetSource(handle)};
        if (source == 0)
            return;

        alSourcef(source, AL_GAIN, std::max(settings.gain, 0.f));
        alSourcef(source, AL_PITCH, std::clamp(settings.pitch, 0.01f, 4.f));
        alSourcei(source, AL_LOOPING, settings.loop ? AL_TRUE : AL_FALSE);

        if (settings.spatial)
        {
            alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);
            alSourcef(source, AL_REFERENCE_DISTANCE, std::max(settings.referenceDistance, 0.f));
            alSourcef(source, AL_MAX_DISTANCE, std::max(settings.maxDistance, settings.referenceDistance));
            alSourcef(source, AL_ROLLOFF_FACTOR, std::max(settings.rolloff, 0.f));
        }
        else
        {
            alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
            alSource3f(source, AL_POSITION, 0.f, 0.f, 0.f);
            alSource3f(source, AL_VELOCITY, 0.f, 0.f, 0.f);
            alSourcef(source, AL_ROLLOFF_FACTOR, 0.f);
        }
    }

    void AudioRegistry::SetSourcePosition(AudioSourceHandle handle, const glm::vec3 &position)
    {
        if (const auto source{GetSource(handle)})
            alSource3f(source, AL_POSITION, position.x, position.y, position.z);
    }

    void AudioRegistry::SetSourceVelocity(AudioSourceHandle handle, const glm::vec3 &velocity)
    {
        if (const auto source{GetSource(handle)})
            alSource3f(source, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    }

    void AudioRegistry::Play(AudioSourceHandle handle)
    {
        if (const auto source{GetSource(handle)})
            alSourcePlay(source);
    }

    void AudioRegistry::Pause(AudioSourceHandle handle)
    {
        if (const auto source{GetSource(handle)})
            alSourcePause(source);
    }

    void AudioRegistry::Stop(AudioSourceHandle handle)
    {
        if (const auto source{GetSource(handle)})
            alSourceStop(source);
    }

    AudioSourceState AudioRegistry::GetState(AudioSourceHandle handle) const
    {
        const auto source{GetSource(handle)};
        if (source == 0)
            return AudioSourceState::Stopped;

        ALint state{};
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        switch (state)
        {
            case AL_PLAYING:
                return AudioSourceState::Playing;
            case AL_PAUSED:
                return AudioSourceState::Paused;
            case AL_INITIAL:
                return AudioSourceState::Initial;
            default:
                return AudioSourceState::Stopped;
        }
    }

    void AudioRegistry::SetListener(const AudioListenerPose &pose)
    {
        if (!Available())
            return;

        alListener3f(AL_POSITION, pose.position.x, pose.position.y, pose.position.z);
        alListener3f(AL_VELOCITY, pose.velocity.x, pose.velocity.y, pose.velocity.z);

        const std::array orientation{
            pose.forward.x, pose.forward.y, pose.forward.z,
            pose.up.x, pose.up.y, pose.up.z
        };
        alListenerfv(AL_ORIENTATION, orientation.data());
    }

    void AudioRegistry::SetListenerGain(float gain)
    {
        if (Available())
            alListenerf(AL_GAIN, std::max(gain, 0.f));
    }

    float AudioRegistry::GetListenerGain() const
    {
        if (!Available())
            return 0.f;

        ALfloat gain{};
        alGetListenerf(AL_GAIN, &gain);
        return gain;
    }

    AudioRegistry::~AudioRegistry()
    {
        Release();
    }

    void AudioRegistry::Release()
    {
        if (mContext)
        {
            alcMakeContextCurrent(mContext.get());

            for (const auto source : mSources)
                if (source != 0)
                {
                    alSourceStop(source);
                    alDeleteSources(1, &source);
                }
            if (!mBuffers.empty())
                alDeleteBuffers(static_cast<ALsizei>(mBuffers.size()), mBuffers.data());
        }

        mSources.clear();
        mFreeSources.clear();
        mBuffers.clear();
        mClipCache.clear();

        mContext.reset();
        mDevice.reset();
    }

    uint32_t AudioRegistry::GetSource(AudioSourceHandle handle) const
    {
        if (!handle.Valid() || handle.index >= mSources.size())
            return 0;
        return mSources[handle.index];
    }

    void AudioRegistry::DeviceDeleter::operator()(ALCdevice *device) const
    {
        alcCloseDevice(device);
    }

    void AudioRegistry::ContextDeleter::operator()(ALCcontext *context) const
    {
        if (alcGetCurrentContext() == context)
            alcMakeContextCurrent(context);
        alcDestroyContext(context);
    }
}
