#include "PointShadowSet.h"

#include "core/components/entt/PendingUploadComponent.h"
#include "core/components/entt/WorldTransform.h"
#include "core/scene/Scene.h"

namespace kailux
{
    void PointShadowSet::Update(const Scene &scene, entt::entity camera)
    {
        const auto selected{Select(scene, camera)};
        const auto previous{mSlots};

        mSlots = {};
        mData = {};

        const auto lights{scene.GetLightData()};
        const auto entities{scene.GetPointLightEntities()};

        uint32_t slot{};
        for (const auto entity: selected)
        {
            if (entity == entt::null)
                continue;

            const auto found{std::ranges::find(entities, entity)};
            if (found == entities.end())
                continue;

            const auto lightIndex{static_cast<uint32_t>(std::distance(entities.begin(), found))};

            auto &current{mSlots[slot]};
            current.setup = PointShadows::build(lights.pointLights[lightIndex], lightIndex);
            current.light = entity;
            current.signature = signature(scene, current.setup);

            const bool sameSlot{previous[slot].light == entity};
            current.dirty = !sameSlot || previous[slot].signature != current.signature;

            auto &gpuSlot{mData.slots[slot]};
            gpuSlot.positionAndFar = glm::vec4(mSlots[slot].setup.position, mSlots[slot].setup.farPlane);
            gpuSlot.params.x = 1.f;
            gpuSlot.lightIndex.x = lightIndex;

            ++slot;
        }

        mData.count.x = slot;
    }

    bool PointShadowSet::NeedsRedraw(uint32_t slot) const
    {
        assert(slot < mSlots.size() && "Point shadow slot out of range");
        return mSlots[slot].setup.Valid() && mSlots[slot].dirty;
    }

    const PointShadowsData & kailux::PointShadowSet::GetData() const
    {
        return mData;
    }

    const glm::mat4 & PointShadowSet::GetFace(uint32_t slot, uint32_t face) const
    {
        assert(slot < mSlots.size() && "Point shadow slot out of range");
        assert(face < details::kPointShadowFaceCount && "Cube face out of range");
        return mSlots[slot].setup.faceViewProjections[face];
    }

    bool PointShadowSet::Enabled(uint32_t slot) const
    {
        assert(slot < mSlots.size() && "Point shadow slot out of range");
        return mSlots[slot].setup.Valid();
    }

    uint32_t PointShadowSet::GetActiveCount() const
    {
        return mData.count.x;
    }

    std::array<entt::entity, details::kMaxPointShadows> PointShadowSet::GetLights() const
    {
        std::array<entt::entity, details::kMaxPointShadows> lights{entt::null};
        for (size_t i{}; i < mSlots.size(); ++i)
            lights[i] = mSlots[i].light;

        return lights;
    }

    uint64_t PointShadowSet::signature(const Scene &scene, const PointShadowSetup &setup)
    {
        const auto mix{[](uint64_t h, uint64_t v)
        {
            return (h ^ (v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2)));
        }};

        uint64_t hash{};
        const auto hashFloat{[&](float f)
        {
            uint32_t bits;
            std::memcpy(&bits, &f, sizeof(uint32_t));
            hash = mix(hash, bits);
        }};

        hashFloat(setup.position.x);
        hashFloat(setup.position.y);
        hashFloat(setup.position.z);
        hashFloat(setup.farPlane);

        const auto &registry{scene.GetEntityRegistry()};
        const auto view{registry.view<WorldTransform, MeshComponent>(entt::exclude<PendingUploadComponent>)};

        for (const auto entity: view)
        {
            const auto &world{view.get<WorldTransform>(entity)};
            const auto sphere{view.get<MeshComponent>(entity).boundingSphere};

            const glm::vec3 center{world.model * glm::vec4(glm::vec3(sphere), 1.f)};
            if (glm::length(center - setup.position) > setup.farPlane + sphere.w)
                continue;

            hash = mix(hash, static_cast<uint64_t>(entity));
            for (int c{}; c < 4; ++c)
                for (int r{}; r < 4; ++r)
                    hashFloat(world.model[c][r]);
        }

        return hash;
    }

    std::array<entt::entity, details::kMaxPointShadows> PointShadowSet::Select(const Scene &scene, entt::entity camera) const
    {
        std::array<entt::entity, details::kMaxPointShadows> selected{entt::null};

        const auto &registry{scene.GetEntityRegistry()};
        if (camera == entt::null || !registry.valid(camera))
            return selected;

        const auto *cameraComponent{registry.try_get<CameraComponent>(camera)};
        if (!cameraComponent)
            return selected;

        const auto lights{scene.GetLightData()};
        const auto entities{scene.GetPointLightEntities()};

        struct Candidate
        {
            entt::entity entity;
            float        score;
        };

        std::vector<Candidate> candidates;
        candidates.reserve(lights.pointLightCount);

        for (uint32_t i{}; i < lights.pointLightCount && i < details::kMaxPointLights; ++i)
        {
            const auto entity{entities[i]};
            if (entity == entt::null)
                continue;

            float score{PointShadows::score(lights.pointLights[i], cameraComponent->position)};
            if (score <= 0.f)
                continue;

            const bool owned{
                std::ranges::any_of(mSlots, [entity](const Slot &slot)
                                    {
                                        return slot.light == entity;
                                    }
                )
            };
            if (owned)
                score *= PointShadows::kStickiness;

            candidates.emplace_back(entity, score);
        }

        const auto keep{std::min<size_t>(candidates.size(), details::kMaxPointShadows)};
        std::ranges::partial_sort(
            candidates,
            candidates.begin() + static_cast<std::ptrdiff_t>(keep),
            [](const Candidate &a, const Candidate &b)
            {
                return a.score > b.score;
            }
        );

        for (size_t i{}; i < keep; ++i)
            selected[i] = candidates[i].entity;

        return selected;
    }
}
