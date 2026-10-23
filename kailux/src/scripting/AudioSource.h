#pragma once

#include <filesystem>
#include <entt/entt.hpp>
#include "audio/AudioRegistry.h"

namespace kailux
{
    class Scene;
    class Entity;
    struct AudioSourceComponent;
    class AudioSource

    {
    public:
        AudioSource();

        bool Valid() const;

        void Play();
        void Pause();
        void Stop();
        bool IsPlaying() const;

        bool             SetClip(const std::filesystem::path& path);
        std::string_view GetClip() const;

        void  SetGain(float gain);
        float GetGain() const;
        void  SetPitch(float pitch);
        float GetPitch() const;
        void  SetLooping(bool loop);
        bool  IsLooping() const;

        const AudioSourceSettings& GetSettings() const;
        void                       SetSettings(const AudioSourceSettings& settings);

    private:
        explicit AudioSource(Scene& scene, entt::entity handle);

        AudioSourceComponent* GetComponent() const;
        AudioRegistry*        GetRegistry() const;
        bool                  Prepare() const;

        Scene*       pScene{};
        entt::entity mHandle{entt::null};

        friend class Entity;
    };
}