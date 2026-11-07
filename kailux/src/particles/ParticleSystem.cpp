#include "ParticleSystem.h"

#include <algorithm>
#include <bit>
#include <cstring>

#include "FrameData.h"
#include "buffer/BufferAllocator.h"
#include "components/entt/ParticleEmitterComponent.h"
#include "components/entt/WorldTransform.h"
#include "scene/Scene.h"

namespace kailux
{
    namespace details
    {
        constexpr float kMaxDeltaTime{0.1f};
        constexpr float kVisibilitySlack{0.1f};

        constexpr float     kBurstLifeMin{0.35f};
        constexpr float     kBurstLifeMax{0.8f};
        constexpr glm::vec3 kBurstGravity{0.f, -7.5f, 0.f};
        constexpr float     kBurstDrag{1.5f};
        constexpr float     kBurstEndSize{0.25f}; // relative to ParticleBurst::size
        constexpr float     kBurstStretch{0.015f};

        static DescriptorSetBufferInfo storage_info(const Buffer& buffer)
        {
            return {
                buffer.GetBuffer(),
                buffer.GetSize(),
                1,
                vk::DescriptorType::eStorageBuffer
            };
        }

        static vk::BufferMemoryBarrier2 buffer_barrier(const Buffer &buffer,
                                                vk::PipelineStageFlags2 srcStage, vk::AccessFlags2 srcAccess,
                                                vk::PipelineStageFlags2 dstStage, vk::AccessFlags2 dstAccess)
        {
            return {
                srcStage,
                srcAccess,
                dstStage,
                dstAccess,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored,
                buffer.GetBuffer(),
                0,
                buffer.GetSize()
            };
        }
    }

    ParticleSystem::ParticleSystem() = default;

    ParticleSystem::ParticleSystem(ParticleSystem &&other) noexcept : mCompute(std::move(other.mCompute)),
                                                                      mPass(std::move(other.mPass)),
                                                                      mParticles(std::move(other.mParticles)),
                                                                      mFrameResources(std::move(other.mFrameResources)),
                                                                      mLayout(std::move(other.mLayout)),
                                                                      mEmitterData(std::move(other.mEmitterData)),
                                                                      mEmitterVisible(std::move(other.mEmitterVisible)),
                                                                      mBurstData(std::move(other.mBurstData)),
                                                                      mQueuedBursts(std::move(other.mQueuedBursts)),
                                                                      mDeltaTime(other.mDeltaTime),
                                                                      mTime(other.mTime),
                                                                      mLastBurstTime(other.mLastBurstTime),
                                                                      mSeed(other.mSeed),
                                                                      mMaxCapacity(other.mMaxCapacity),
                                                                      mSimulating(other.mSimulating),
                                                                      mClearPending(other.mClearPending)
    {
    }

    ParticleSystem &ParticleSystem::operator=(ParticleSystem &&other) noexcept
    {
        if (this != &other)
        {
            mCompute = std::move(other.mCompute);
            mPass = std::move(other.mPass);
            mParticles = std::move(other.mParticles);
            mFrameResources = std::move(other.mFrameResources);
            mLayout = std::move(other.mLayout);
            mEmitterData = std::move(other.mEmitterData);
            mEmitterVisible = std::move(other.mEmitterVisible);
            mBurstData = std::move(other.mBurstData);
            mQueuedBursts = std::move(other.mQueuedBursts);
            mDeltaTime = other.mDeltaTime;
            mTime = other.mTime;
            mLastBurstTime = other.mLastBurstTime;
            mSeed = other.mSeed;
            mMaxCapacity = other.mMaxCapacity;
            mSimulating = other.mSimulating;
            mClearPending = other.mClearPending;
        }
        return *this;
    }

    ParticleSystem ParticleSystem::create(const Context &context, const Swapchain &swapchain,
        std::span<FrameData> frames)
    {
        ParticleSystem system;
        system.mCompute = ComputeParticle::create(context, details::kFramesInFlight);
        system.mPass = ParticlePass::create(context, swapchain, details::kFramesInFlight);
        system.mParticles = buffer_allocator::alloc_local(
            context,
            sizeof(ParticleData) * kMaxParticles,
            vk::BufferUsageFlagBits::eStorageBuffer
            );
        system.CreateFrameResources(context, frames);
        system.mClearPending = true;
        return system;
    }

    void ParticleSystem::Update(Scene &scene, float deltaTime, bool simulating)
    {
        auto &registry{scene.GetEntityRegistry()};

        mSimulating = simulating;
        mDeltaTime  = simulating ? std::min(deltaTime, details::kMaxDeltaTime) : 0.f;
        mTime      += mDeltaTime;

        if (UpdateLayout(registry))
            mClearPending = true;

        mEmitterData.assign(mLayout.size(), ParticleEmitterData{});
        mEmitterVisible.assign(mLayout.size(), false);
        mBurstData.clear();
        mMaxCapacity = 0;

        uint32_t offset{};

        // burst pool
        {
            uint32_t spawn{};
            if (simulating)
            {
                const auto count{std::min<size_t>(mQueuedBursts.size(), kMaxBurstsPerFrame)};
                for (size_t i{}; i < count && spawn < kBurstPoolCapacity; ++i)
                {
                    auto burst{mQueuedBursts[i]};
                    spawn = std::min(spawn + burst.info.x, kBurstPoolCapacity);
                    burst.info.x = spawn;
                    mBurstData.push_back(burst);
                }
            }
            mQueuedBursts.clear();

            if (spawn > 0)
                mLastBurstTime = mTime;

            auto &data{mEmitterData[0]};
            data.positionAndShape     = {0.f, 0.f, 0.f, static_cast<float>(ParticleShape::Point)};
            data.boxExtentsAndSpread  = {0.f, 0.f, 0.f, -1.f};
            data.directionAndDrag     = {0.f, 1.f, 0.f, details::kBurstDrag};
            data.gravityAndTurbulence = {details::kBurstGravity, 0.f};
            data.speedAndLife         = {0.f, 0.f, details::kBurstLifeMin, details::kBurstLifeMax};
            data.startColor           = {1.f, 1.f, 1.f, 1.f};
            data.endColor             = {1.f, 1.f, 1.f, 0.f};
            data.sizeAndStretch       = {1.f, details::kBurstEndSize, details::kBurstStretch, 1.f};
            data.range                = {offset, kBurstPoolCapacity, spawn, 0u};
            data.bursts               = {0u, static_cast<uint32_t>(mBurstData.size()), 0u, 0u};

            mEmitterVisible[0] = mLastBurstTime >= 0.f && mTime - mLastBurstTime <= details::kBurstLifeMax + details::kVisibilitySlack;
            offset += kBurstPoolCapacity;
            mMaxCapacity = kBurstPoolCapacity;
        }

        for (size_t i{1}; i < mLayout.size(); ++i)
        {
            const auto [entity, capacity]{mLayout[i]};
            auto       &emitter{registry.get<ParticleEmitterComponent>(entity)};
            const auto &world{registry.get<WorldTransform>(entity)};
            const auto &settings{emitter.settings};

            uint32_t spawn{};
            if (simulating)
            {
                if (settings.emitting)
                {
                    emitter.spawnAccumulator += std::max(settings.rate, 0.f) * mDeltaTime;
                    const auto whole{std::floor(emitter.spawnAccumulator)};
                    emitter.spawnAccumulator -= whole;
                    spawn = static_cast<uint32_t>(std::min(whole, static_cast<float>(capacity)));
                }
                spawn = std::min(spawn + emitter.pendingBurst, capacity);
            }
            else
                emitter.spawnAccumulator = 0.f;
            emitter.pendingBurst = 0;

            if (spawn > 0)
                emitter.lastSpawnTime = mTime;

            const auto lifeMin{std::max(settings.lifeMin, 0.01f)};
            const auto lifeMax{std::max(settings.lifeMax, lifeMin)};

            glm::vec3 direction{glm::mat3(world.model) * settings.direction};
            const auto length{glm::length(direction)};
            direction = length > 1e-5f ? direction / length : glm::vec3{0.f, 1.f, 0.f};

            const auto spread{glm::radians(std::clamp(settings.spread, 0.f, 180.f))};

            uint32_t flags{};
            if (settings.confineToBox)
                flags |= ParticleEmitterData::kFlagConfineToBox;

            auto &data{mEmitterData[i]};
            data.positionAndShape     = {world.GetPosition(), static_cast<float>(settings.shape)};
            data.boxExtentsAndSpread  = {glm::max(settings.boxExtents, glm::vec3{0.f}), std::cos(spread)};
            data.directionAndDrag     = {direction, std::max(settings.drag, 0.f)};
            data.gravityAndTurbulence = {settings.gravity, std::max(settings.turbulence, 0.f)};
            data.speedAndLife         = {settings.speedMin, std::max(settings.speedMax, settings.speedMin), lifeMin, lifeMax};
            data.startColor           = settings.startColor;
            data.endColor             = settings.endColor;
            data.sizeAndStretch       = {
                std::max(settings.startSize, 0.f),
                std::max(settings.endSize, 0.f),
                std::max(settings.stretch, 0.f),
                settings.additive ? 1.f : 0.f
            };
            data.range  = {offset, capacity, spawn, flags};
            data.bursts = {0u, 0u, 0u, 0u};

            mEmitterVisible[i] = emitter.lastSpawnTime >= 0.f && mTime - emitter.lastSpawnTime <= lifeMax + details::kVisibilitySlack;
            offset += capacity;
            mMaxCapacity = std::max(mMaxCapacity, capacity);
        }
    }

    void ParticleSystem::QueueBurst(const glm::vec3 &position, const ParticleBurst &burst)
    {
        if (burst.count == 0 || mQueuedBursts.size() >= kMaxBurstsPerFrame)
            return;

        ParticleBurstData data;
        data.positionAndSpeed = {position, std::max(burst.speed, 0.f)};
        data.color              = glm::clamp(burst.color, glm::vec4{0.f}, glm::vec4{1.f});
        data.info               = {
            std::min(burst.count, kBurstPoolCapacity),
            std::bit_cast<uint32_t>(std::max(burst.size, 0.f)),
            0u,
            0u
        };
        mQueuedBursts.push_back(data);
    }

    void ParticleSystem::Reset()
    {
        mClearPending  = true;
        mLastBurstTime = -1.f;
        mQueuedBursts.clear();
        mLayout.clear();
    }

    void ParticleSystem::Simulate(const CommandRecorder &recorder, uint32_t frameIndex)
    {
        const auto& resources{mFrameResources[frameIndex]};
        const auto cmd{recorder.GetCommandBuffer()};

        const auto emitterCount{static_cast<uint32_t>(mEmitterData.size())};
        if (emitterCount > 0)
            std::memcpy(resources.emitters.Mapped<ParticleEmitterData>().data(), mEmitterData.data(),
                        sizeof(ParticleEmitterData) * emitterCount);
        if (!mBurstData.empty())
            std::memcpy(resources.bursts.Mapped<ParticleBurstData>().data(), mBurstData.data(),
                        sizeof(ParticleBurstData) * mBurstData.size());
        std::ranges::fill(resources.spawnCounters.Mapped<uint32_t>(), 0u);

        if (mClearPending)
        {
            mClearPending = false;

            const std::array beforeClear{
                details::buffer_barrier(mParticles,
                    vk::PipelineStageFlagBits2::eVertexShader | vk::PipelineStageFlagBits2::eComputeShader,
                    vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite,
                    vk::PipelineStageFlagBits2::eTransfer,
                    vk::AccessFlagBits2::eTransferWrite)
            };
            recorder.BufferMemoryBarriers(beforeClear);

            cmd.fillBuffer(mParticles.GetBuffer(), 0, mParticles.GetSize(), 0);

            const std::array afterClear{
                details::buffer_barrier(mParticles,
                               vk::PipelineStageFlagBits2::eTransfer,
                               vk::AccessFlagBits2::eTransferWrite,
                               vk::PipelineStageFlagBits2::eVertexShader | vk::PipelineStageFlagBits2::eComputeShader,
                               vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite)
            };
            recorder.BufferMemoryBarriers(afterClear);
        }

        if (!mSimulating || emitterCount == 0 || mMaxCapacity == 0)
            return;

        const std::array beforeSimulation{
            details::buffer_barrier(mParticles,
                                    vk::PipelineStageFlagBits2::eVertexShader,
                                    vk::AccessFlagBits2::eShaderStorageRead,
                                    vk::PipelineStageFlagBits2::eComputeShader,
                                    vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite)
        };
        recorder.BufferMemoryBarriers(beforeSimulation);

        mSeed += 0x9e3779b9u;

        mCompute.Bind(cmd);
        resources.computeSet.Bind(mCompute.GetPipeline(), cmd);
        mCompute.Push<compute_pass_push_constants::ParticleSimulation>(
            cmd,
            {
                mDeltaTime,
                mTime,
                mSeed,
                emitterCount
            }
        );
        mCompute.Execute(cmd,
                         {
                             (mMaxCapacity + ComputeParticle::kWorkgroupSize - 1) / ComputeParticle::kWorkgroupSize,
                             emitterCount,
                             1
                         }
        );

        const std::array afterSimulation{
            details::buffer_barrier(mParticles,
                                    vk::PipelineStageFlagBits2::eComputeShader,
                                    vk::AccessFlagBits2::eShaderStorageWrite,
                                    vk::PipelineStageFlagBits2::eVertexShader | vk::PipelineStageFlagBits2::eComputeShader,
                                    vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite)
        };
        recorder.BufferMemoryBarriers(afterSimulation);
    }

    void ParticleSystem::Draw(vk::CommandBuffer cmd, uint32_t frameIndex, uint32_t cameraIndex, bool multisampled) const
    {
        if (std::ranges::none_of(mEmitterVisible, [](bool visible){return visible;}))
            return;

        mPass.Bind(cmd, multisampled);
        mFrameResources[frameIndex].drawSet.Bind(mPass.GetPipeline(), cmd);

        for (uint32_t i{}; i < mEmitterData.size(); ++i)
        {
            const auto& range{mEmitterData[i].range};
            if (!mEmitterVisible[i] || range.y == 0)
                continue;

            mPass.Push<graphics_pass_push_constants::Particles>(
                cmd,
                {
                    i,
                    cameraIndex
                }
            );
            cmd.draw(6, range.y, 0, range.x);
        }
    }

    uint32_t ParticleSystem::GetAllocatedParticles() const
    {
        uint32_t total{};
        for (const auto &allocation : mLayout)
            total += allocation.capacity;
        return total;
    }

    void ParticleSystem::CreateFrameResources(const Context &context, std::span<FrameData> frames)
    {
        for (uint32_t i{}; i < details::kFramesInFlight; ++i)
        {
            auto &resources{mFrameResources[i]};
            resources.emitters      = buffer_allocator::alloc_storage(context, sizeof(ParticleEmitterData) * kMaxEmitters);
            resources.bursts        = buffer_allocator::alloc_storage(context, sizeof(ParticleBurstData) * kMaxBurstsPerFrame);
            resources.spawnCounters = buffer_allocator::alloc_storage(context, sizeof(uint32_t) * kMaxEmitters);

            const std::array<DescriptorSetInfo, 4> computeInfos{
                details::storage_info(mParticles),
                details::storage_info(resources.emitters),
                details::storage_info(resources.bursts),
                details::storage_info(resources.spawnCounters)
            };
            resources.computeSet = DescriptorSet::create(
                context,
                mCompute.GetDescriptorLayout(),
                mCompute.GetDescriptorPool(),
                computeInfos
            );

            const auto &camera{frames[i].GetCameraBuffer()};
            const std::array<DescriptorSetInfo, 3> drawInfos{
                DescriptorSetBufferInfo{camera.GetBuffer(), camera.GetSize(), 1, vk::DescriptorType::eUniformBuffer},
                details::storage_info(mParticles),
                details::storage_info(resources.emitters)
            };
            resources.drawSet = DescriptorSet::create(
                context,
                mPass.GetDescriptorLayout(),
                mPass.GetDescriptorPool(),
                drawInfos
            );
        }
    }

    bool ParticleSystem::UpdateLayout(const entt::registry &registry)
    {
        std::vector<entt::entity> entities;
        for (const auto entity : registry.view<ParticleEmitterComponent, WorldTransform>())
            entities.push_back(entity);

        std::ranges::sort(entities, {}, [](auto entity)
        {
            return entt::to_integral(entity);
        });

        std::vector<Allocation> layout;
        layout.reserve(std::min<size_t>(entities.size() + 1, kMaxEmitters));
        layout.push_back({entt::null, kBurstPoolCapacity});

        uint32_t remaining{kMaxParticles - kBurstPoolCapacity};
        for (const auto entity : entities)
        {
            if (layout.size() >= kMaxEmitters || remaining == 0)
                break;

            const auto requested{std::max(registry.get<ParticleEmitterComponent>(entity).settings.maxParticles, 1u)};
            const auto capacity{std::min(requested, remaining)};
            remaining -= capacity;
            layout.push_back({entity, capacity});
        }

        const bool changed{layout != mLayout};
        mLayout = std::move(layout);
        return changed;
    }
}
