#pragma once

#include <entt/entt.hpp>
#include "particles/ParticleTypes.h"

namespace kailux
{
    class Scene;
    class Entity;
    struct ParticleEmitterComponent;

    class ParticleEmitter
    {
    public:
        ParticleEmitter();

        bool Valid() const;

        void Emit(uint32_t count) const;

        void SetEmitting(bool emitting) const;
        bool IsEmitting() const;

        void  SetRate(float rate) const;
        float GetRate() const;

        const ParticleEmitterSettings& GetSettings() const;
        void                           SetSettings(const ParticleEmitterSettings& settings) const;

    private:
        explicit ParticleEmitter(Scene& scene, entt::entity handle);

        ParticleEmitterComponent* GetComponent() const;

        Scene*       pScene{};
        entt::entity mHandle{entt::null};

        friend class Entity;
    };
}