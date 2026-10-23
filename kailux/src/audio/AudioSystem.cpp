#include "AudioSystem.h"
#include "Log.h"
#include "scene/Scene.h"
#include  "components/entt/AudioSourceComponent.h"
#include "components/entt/PhysicsControlComponent.h"
#include "components/entt/WorldTransform.h"

namespace kailux
{
    AudioSystem::AudioSystem(AudioRegistry &audioRegistry) : mAudioRegistryRef(audioRegistry),
                                                             mOnInfoLog([](auto message)
                                                             {
                                                                 log::console.Info("{}", message);
                                                             }),
                                                             mOnWarningLog([](auto message)
                                                             {
                                                                 log::console.Warning("{}", message);
                                                             })
    {
    }

    void AudioSystem::Update(Scene &scene, float deltaTime)
    {
        if (mSimulationState != SimulationState::Running
            || !mAudioRegistryRef.Available())
            return;

        UpdateListener(scene, deltaTime);
        UpdateSources(scene, deltaTime);
    }

    void AudioSystem::SetSimulationState(Scene &scene, SimulationState state)
    {
        if (state == mSimulationState)
            return;

        mSimulationState = state;
        if (state == SimulationState::Running)
            OnSimulationStart(scene);
        else
            OnSimulationStop(scene);
    }

    SimulationState AudioSystem::GetSimulationState() const
    {
        return mSimulationState;
    }

    namespace details
    {
        static void on_audio_source_destroyed(entt::registry &registry, entt::entity entity)
        {
            const auto *audio{registry.ctx().find<AudioRegistry *>()};
            if (!audio || !*audio)
                return;

            const auto &component{registry.get<AudioSourceComponent>(entity)};
            (*audio)->DestroySource(component.source);
        }

        static std::string clip_key(const AudioSourceComponent &component)
        {
            return component.settings.spatial ? component.clip + "#mono" : component.clip;
        }
    }

    void AudioSystem::AttachToScene(Scene &scene) const
    {
        auto &registry{scene.GetEntityRegistry()};
        registry.ctx().insert_or_assign<AudioRegistry *>(&mAudioRegistryRef);

        auto sink{registry.on_destroy<AudioSourceComponent>()};
        sink.disconnect<&details::on_audio_source_destroyed>();
        sink.connect<&details::on_audio_source_destroyed>();
    }

    void AudioSystem::DetachScene(Scene &scene)
    {
        for (auto &&[entity, component]
             : scene.GetEntityRegistry().view<AudioSourceComponent>().each())
        {
            mAudioRegistryRef.DestroySource(component.source);
            component.source = {};
            component.loadedClip.clear();
            component.resumeOnStart = false;
        }
        StopOneShots();
    }

    bool AudioSystem::PlayOneShot(const std::filesystem::path &path, std::optional<glm::vec3> position, float gain)
    {
        if (!mAudioRegistryRef.Available())
            return false;

        const auto clip{mAudioRegistryRef.LoadClip(path, position.has_value())};
        if (!clip)
        {
            if (mFailedOneShots.insert(path.generic_string()).second)
                mOnWarningLog(clip.error());
            return false;
        }

        const auto source{AcquireOneShot()};
        if (!source.Valid())
            return false;

        AudioSourceSettings settings;
        settings.gain = gain;
        settings.spatial = position.has_value();

        mAudioRegistryRef.SetSourceClip(source, *clip);
        mAudioRegistryRef.SetSourceSettings(source, settings);
        if (position)
        {
            mAudioRegistryRef.SetSourcePosition(source, *position);
            mAudioRegistryRef.SetSourceVelocity(source, glm::vec3{0.f});
        }
        mAudioRegistryRef.Play(source);
        return true;
    }

    void AudioSystem::SetMasterVolume(float volume)
    {
        mMasterVolume = std::max(volume, 0.f);
        mAudioRegistryRef.SetListenerGain(mMasterVolume);
    }

    float AudioSystem::GetMasterVolume() const
    {
        return mMasterVolume;
    }

    AudioSystem::PrepareResult AudioSystem::prepare_source(AudioRegistry &audioRegistry,
                                                           AudioSourceComponent &component)
    {
        if (!audioRegistry.IsSourceValid(component.source))
        {
            const auto source{audioRegistry.CreateSource()};
            if (!source)
                return std::unexpected(source.error());
            component.source = *source;
            component.loadedClip.clear();
        }

        if (component.clip.empty())
        {
            if (!component.loadedClip.empty())
            {
                audioRegistry.SetSourceClip(component.source, {});
                component.loadedClip.clear();
            }
        } else if (auto key{details::clip_key(component)}; key != component.loadedClip)
        {
            const auto clip{audioRegistry.LoadClip(component.clip, component.settings.spatial)};
            if (!clip)
                return std::unexpected(clip.error());
            audioRegistry.SetSourceClip(component.source, *clip);
            component.loadedClip = std::move(key);
        }

        audioRegistry.SetSourceSettings(component.source, component.settings);
        return {};
    }

    void AudioSystem::SetOnInfoLog(OnLog &&callback)
    {
        mOnInfoLog = std::move(callback);
    }

    void AudioSystem::SetOnWarningLog(OnLog &&callback)
    {
        mOnWarningLog = std::move(callback);
    }

    void AudioSystem::OnSimulationStart(Scene &scene)
    {
        mAudioRegistryRef.SetListenerGain(mMasterVolume);
        mHasListenerPosition = false;
        mFailedOneShots.clear();

        for (auto &&[entity, component]
             : scene.GetEntityRegistry().view<AudioSourceComponent>().each())
        {
            component.hasLastPosition = false;
            component.loadFailed = false;

            const bool resume{std::exchange(component.resumeOnStart, false)};
            if (!mAudioRegistryRef.IsSourceValid(component.source))
                continue;

            if (const auto result{prepare_source(mAudioRegistryRef, component)}; !result)
            {
                mOnWarningLog(result.error());
                component.loadFailed = true;
                continue;
            }

            if (resume)
                mAudioRegistryRef.Play(component.source);
            else
            {
                mAudioRegistryRef.Stop(component.source);
                if (component.settings.playOnStart && !component.clip.empty())
                    mAudioRegistryRef.Play(component.source);
            }
        }
    }

    void AudioSystem::OnSimulationStop(Scene &scene)
    {
        for (auto &&[entity, component]
             : scene.GetEntityRegistry().view<AudioSourceComponent>().each())
        {
            component.resumeOnStart = mAudioRegistryRef.GetState(component.source) == AudioSourceState::Playing;
            if (component.resumeOnStart)
                mAudioRegistryRef.Pause(component.source);
        }
        StopOneShots();
    }

    void AudioSystem::UpdateListener(Scene &scene, float deltaTime)
    {
        auto& registry{scene.GetEntityRegistry()};
        const auto camera{scene.GetPrimaryCamera()};
        if (camera == entt::null || !registry.valid(camera))
            return;

        const auto* component{registry.try_get<CameraComponent>(camera)};
        if (!component)
            return;

        AudioListenerPose pose;
        pose.position = component->position;
        pose.forward  = component->forward;
        pose.up       = component->up;
        if (mHasListenerPosition && deltaTime > 0.f)
            pose.velocity = clamp_velocity((pose.position - mLastListenerPosition) / deltaTime);

        mLastListenerPosition = pose.position;
        mHasListenerPosition  = true;

        mAudioRegistryRef.SetListener(pose);
    }

    void AudioSystem::UpdateSources(Scene &scene, float deltaTime)
    {
        auto& registry{scene.GetEntityRegistry()};
        for (auto &&[entity, component]
             : registry.view<AudioSourceComponent>().each())
        {
            if (!mAudioRegistryRef.IsSourceValid(component.source))
            {
                if (component.loadFailed)
                    continue;

                if (const auto result{prepare_source(mAudioRegistryRef, component)}; !result)
                {
                    mOnWarningLog(result.error());
                    component.loadFailed = true;
                    continue;
                }

                if (component.settings.playOnStart && !component.clip.empty())
                    mAudioRegistryRef.Play(component.source);
            }

            if (!component.settings.spatial)
                continue;

            const auto* transform{registry.try_get<WorldTransform>(entity)};
            if (!transform)
                continue;

            const auto position{transform->GetPosition()};

            glm::vec3 velocity{0.f};
            const auto* physics{registry.try_get<PhysicsComponent>(entity)};
            const auto* control{registry.try_get<PhysicsControlComponent>(entity)};
            if (physics && physics->IsDynamic() && control)
                velocity = control->velocity;

            else if (component.hasLastPosition && deltaTime > 0.f)
                velocity = (position - component.lastPosition) / deltaTime;

            component.lastPosition    = position;
            component.hasLastPosition = true;

            mAudioRegistryRef.SetSourcePosition(component.source, position);
            mAudioRegistryRef.SetSourceVelocity(component.source, clamp_velocity(velocity));
        }
    }

    AudioSourceHandle AudioSystem::AcquireOneShot()
    {
        for (const auto source : mOneShots)
        {
            const auto state{mAudioRegistryRef.GetState(source)};
            if (state == AudioSourceState::Initial || state == AudioSourceState::Stopped)
                return source;
        }

        if (mOneShots.size() < kMaxOneShots)
        {
            const auto source{mAudioRegistryRef.CreateSource()};
            if (!source)
            {
                mOnWarningLog(source.error());
                return {};
            }
            mOneShots.push_back(*source);
            return *source;
        }

        const auto stolen{mOneShots[mNextOneShot]};
        mNextOneShot = (mNextOneShot + 1) % kMaxOneShots;
        mAudioRegistryRef.Stop(stolen);
        return stolen;
    }

    void AudioSystem::StopOneShots()
    {
        for (const auto source : mOneShots)
            mAudioRegistryRef.Stop(source);
    }

    glm::vec3 AudioSystem::clamp_velocity(const glm::vec3 &velocity)
    {
        const auto speed{glm::length(velocity)};
        if (speed <= kMaxDopplerSpeed || speed <= 0.f)
            return velocity;
        return  velocity * (kMaxDopplerSpeed / speed);
    }
}
