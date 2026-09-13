#include "PointShadowSet.h"
#include "core/scene/Scene.h"

namespace kailux
{
    void PointShadowSet::Update(const Scene &scene, entt::entity camera)
    {
        const auto selected{Select(scene, camera)};

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

            mSlots[slot].setup = PointShadows::build(lights.pointLights[lightIndex], lightIndex);
            mSlots[slot].light = entity;

            auto &gpuSlot{mData.slots[slot]};
            gpuSlot.positionAndFar = glm::vec4(mSlots[slot].setup.position, mSlots[slot].setup.farPlane);
            gpuSlot.params.x = 1.f;
            gpuSlot.lightIndex.x = lightIndex;

            ++slot;
        }

        mData.count.x = slot;
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
