#include "AudioSource.h"

#include "Log.h"
#include "audio/AudioSystem.h"
#include "components/entt/AudioSourceComponent.h"
#include "components/entt/TagComponent.h"
#include "scene/Scene.h"

namespace kailux
{
    AudioSource::AudioSource() = default;

    bool AudioSource::Valid() const
    {
        return GetComponent() != nullptr;
    }

    void AudioSource::Play()
    {
        if (Prepare())
            GetRegistry()->Play(GetComponent()->source);
    }

    void AudioSource::Pause()
    {
        if (const auto* component{GetComponent()}; component && GetRegistry())
            GetRegistry()->Pause(component->source);
    }

    void AudioSource::Stop()
    {
        if (const auto* component{GetComponent()}; component && GetRegistry())
            GetRegistry()->Stop(component->source);
    }

    bool AudioSource::IsPlaying() const
    {
        const auto* component{GetComponent()};
        const auto* audio{GetRegistry()};
        return component
               && audio
               && audio->GetState(component->source) == AudioSourceState::Playing;
    }

    bool AudioSource::SetClip(const std::filesystem::path &path)
    {
        auto* component{GetComponent()};
        if (!component)
            return false;

        component->clip       = path.generic_string();
        component->loadFailed = false;
        return Prepare();
    }

    std::string_view AudioSource::GetClip() const
    {
        if (const auto* component{GetComponent()})
            return component->clip;
        return {};
    }

    void AudioSource::SetGain(float gain)
    {
        auto settings{GetSettings()};
        settings.gain = gain;
        SetSettings(settings);
    }

    float AudioSource::GetGain() const
    {
        return GetSettings().gain;
    }

    void AudioSource::SetPitch(float pitch)
    {
        auto settings{GetSettings()};
        settings.pitch = pitch;
        SetSettings(settings);
    }

    float AudioSource::GetPitch() const
    {
        return GetSettings().pitch;
    }

    void AudioSource::SetLooping(bool loop)
    {
        auto settings{GetSettings()};
        settings.loop = loop;
        SetSettings(settings);
    }

    bool AudioSource::IsLooping() const
    {
        return GetSettings().loop;
    }

    const AudioSourceSettings &AudioSource::GetSettings() const
    {
        static constexpr AudioSourceSettings kDefault{};
        if (const auto* component{GetComponent()})
            return component->settings;
        return kDefault;
    }

    void AudioSource::SetSettings(const AudioSourceSettings &settings)
    {
        auto* component{GetComponent()};
        if (!component)
            return;

        component->settings = settings;
        if (auto* audio{GetRegistry()}; audio && audio->IsSourceValid(component->source))
            Prepare();
    }

    AudioSource::AudioSource(Scene &scene, entt::entity handle) : pScene(&scene),
                                                                  mHandle(handle)
    {
    }

    AudioSourceComponent *AudioSource::GetComponent() const
    {
        if (!pScene || !pScene->GetEntityRegistry().valid(mHandle))
            return nullptr;
        return pScene->GetEntityRegistry().try_get<AudioSourceComponent>(mHandle);
    }

    AudioRegistry *AudioSource::GetRegistry() const
    {
        if (!pScene)
            return nullptr;

        auto* const* registry{pScene->GetEntityRegistry().ctx().find<AudioRegistry*>()};
        return registry ? *registry : nullptr;
    }

    bool AudioSource::Prepare() const
    {
        auto* component{GetComponent()};
        auto* audio{GetRegistry()};
        if (!component || !audio || !audio->Available())
            return false;

        if (const auto result{AudioSystem::prepare_source(*audio, *component)}; !result)
        {
            const auto* tag{pScene->GetEntityRegistry().try_get<TagComponent>(mHandle)};
            log::console.Warning("Audio source on '{}': {}", tag ? tag->name : std::string{}, result.error());
            component->loadFailed = true;
            return false;
        }

        component->loadFailed = false;
        return true;
    }
}
