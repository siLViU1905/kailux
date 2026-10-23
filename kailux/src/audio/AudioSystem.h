#pragma once

#include <optional>
#include <unordered_set>

#include "AudioRegistry.h"

namespace kailux
{
    class Scene;
    struct AudioSourceComponent;

    class AudioSystem
    {
    public:
        explicit AudioSystem(AudioRegistry& audioRegistry);

        void Update(Scene& scene, float deltaTime);

        void            SetSimulationState(Scene& scene, SimulationState state);
        SimulationState GetSimulationState() const;

        void AttachToScene(Scene& scene) const;
        void DetachScene(Scene& scene);

        bool PlayOneShot(const std::filesystem::path& path, std::optional<glm::vec3> position, float gain);

        void  SetMasterVolume(float volume);
        float GetMasterVolume() const;

        using PrepareResult = std::expected<void, std::string>;
        static PrepareResult prepare_source(AudioRegistry& audioRegistry, AudioSourceComponent& component);

        using OnLog = std::move_only_function<void(std::string_view)>;
        void SetOnInfoLog(OnLog&& callback);
        void SetOnWarningLog(OnLog&& callback);

    private:
        static constexpr uint32_t kMaxOneShots{32};
        static constexpr float    kMaxDopplerSpeed{100.f};

        void OnSimulationStart(Scene& scene);
        void OnSimulationStop(Scene& scene);

        void UpdateListener(Scene& scene, float deltaTime);
        void UpdateSources(Scene& scene, float deltaTime);

        AudioSourceHandle AcquireOneShot();
        void              StopOneShots();

        static glm::vec3 clamp_velocity(const glm::vec3& velocity);

        AudioRegistry&  mAudioRegistryRef;

        SimulationState mSimulationState{SimulationState::Paused};

        std::vector<AudioSourceHandle>  mOneShots;
        uint32_t                        mNextOneShot{};
        std::unordered_set<std::string> mFailedOneShots;

        glm::vec3 mLastListenerPosition{0.f};
        bool      mHasListenerPosition{};
        float     mMasterVolume{1.f};

        OnLog mOnInfoLog;
        OnLog mOnWarningLog;
    };
}
