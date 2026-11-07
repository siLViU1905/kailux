#pragma once

#include <entt/entt.hpp>

#include "Core.h"
#include "ParticleTypes.h"
#include "buffer/Buffer.h"
#include "command/CommandRecorder.h"
#include "components/gpu/ParticleData.h"
#include "descriptor/DescriptorSet.h"
#include "passes/ComputeParticle.h"
#include "passes/ParticlePass.h"

namespace kailux
{
    class Scene;
    class FrameData;

    class ParticleSystem
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ParticleSystem)

        static ParticleSystem create(const Context &context, const Swapchain &swapchain, std::span<FrameData> frames);

        static constexpr uint32_t kMaxParticles{1u << 20};     // 48 MB of particle state
        static constexpr uint32_t kMaxEmitters{64};            // the burst pool included
        static constexpr uint32_t kMaxBurstsPerFrame{128};
        static constexpr uint32_t kBurstPoolCapacity{16'384};

        void Update(Scene &scene, float deltaTime, bool simulating);

        void QueueBurst(const glm::vec3 &position, const ParticleBurst &burst);

        void Reset();

        void Simulate(const CommandRecorder &recorder, uint32_t frameIndex);

        void Draw(vk::CommandBuffer cmd, uint32_t frameIndex, uint32_t cameraIndex, bool multisampled) const;

        uint32_t GetAllocatedParticles() const;

    private:
        struct FrameResources
        {
            Buffer        emitters;
            Buffer        bursts;
            Buffer        spawnCounters;
            DescriptorSet computeSet;
            DescriptorSet drawSet;
        };

        struct Allocation
        {
            entt::entity entity{entt::null};
            uint32_t     capacity{};

            bool operator==(const Allocation &) const = default;
        };

        void CreateFrameResources(const Context &context, std::span<FrameData> frames);
        bool UpdateLayout(const entt::registry &registry);

        ComputeParticle mCompute;
        ParticlePass    mPass;
        Buffer          mParticles;

        std::array<FrameResources, details::kFramesInFlight> mFrameResources;

        std::vector<Allocation>          mLayout;           // index 0 is the burst pool
        std::vector<ParticleEmitterData> mEmitterData;      // matches mLayout
        std::vector<bool>                mEmitterVisible;   // matches mLayout
        std::vector<ParticleBurstData>   mBurstData;
        std::vector<ParticleBurstData>   mQueuedBursts;

        float    mDeltaTime{};
        float    mTime{};
        float    mLastBurstTime{-1.f};
        uint32_t mSeed{};
        uint32_t mMaxCapacity{};
        bool     mSimulating{};
        bool     mClearPending{true};
    };
}