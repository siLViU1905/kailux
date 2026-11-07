#include "ParticleEmitter.h"

#include "components/entt/ParticleEmitterComponent.h"
#include "scene/Scene.h"

namespace kailux
{
    ParticleEmitter::ParticleEmitter() = default;

    bool ParticleEmitter::Valid() const
    {
        return GetComponent() != nullptr;
    }

    void ParticleEmitter::Emit(uint32_t count) const
    {
        if (auto* component{GetComponent()})
            component->pendingBurst += count;
    }

    void ParticleEmitter::SetEmitting(bool emitting) const
    {
        if (auto* component{GetComponent()})
            component->settings.emitting = emitting;
    }

    bool ParticleEmitter::IsEmitting() const
    {
        return GetSettings().emitting;
    }

    void ParticleEmitter::SetRate(float rate) const
    {
        if (auto* component{GetComponent()})
            component->settings.rate = rate;
    }

    float ParticleEmitter::GetRate() const
    {
        return GetSettings().rate;
    }

    const ParticleEmitterSettings &ParticleEmitter::GetSettings() const
    {
        static const ParticleEmitterSettings kDefault{};
        if (const auto* component{GetComponent()})
            return component->settings;
        return kDefault;
    }

    void ParticleEmitter::SetSettings(const ParticleEmitterSettings &settings) const
    {
        if (auto* component{GetComponent()})
            component->settings = settings;
    }

    ParticleEmitter::ParticleEmitter(Scene &scene, entt::entity handle) : pScene(&scene),
                                                                          mHandle(handle)
    {
    }

    ParticleEmitterComponent *ParticleEmitter::GetComponent() const
    {
        if (!pScene || !pScene->GetEntityRegistry().valid(mHandle))
            return nullptr;
        return pScene->GetEntityRegistry().try_get<ParticleEmitterComponent>(mHandle);
    }
}
