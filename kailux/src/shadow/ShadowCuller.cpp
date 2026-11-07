#include "ShadowCuller.h"

#include "Camera.h"

namespace kailux
{
    ShadowCuller::ShadowCuller(FrameData &frame, const MeshRegistry &meshRegistry,
                               std::span<const PointShadowSet, details::kMaxCameraViews> pointSets) : mMeshRegistry(
            meshRegistry),
        mPointCommands(frame.GetPointShadowCommandsBuffer().Mapped<vk::DrawIndexedIndirectCommand>()),
        mPointDrawCounts(frame.GetMeshUploadState().pointShadowDrawCounts)
    {
        mPointDrawCounts.fill(0);

        for (uint32_t viewIndex{}; viewIndex < details::kMaxCameraViews; ++viewIndex)
        {
            const auto &shadowSet{pointSets[viewIndex]};
            for (uint32_t slot{}; slot < details::kMaxPointShadows; ++slot)
            {
                if (!shadowSet.NeedsRedraw(slot))
                    continue;

                auto &target{mPointTargets[mPointTargetCount++]};
                const auto positionAndFar{shadowSet.GetData().slots[slot].positionAndFar};
                target.view = viewIndex;
                target.slot = slot;
                target.position = glm::vec3(positionAndFar);
                target.farPlane = positionAndFar.w;

                for (uint32_t face{}; face < details::kPointShadowFaceCount; ++face)
                    target.facePlanes[face] = camera::get_frustum_planes(shadowSet.GetFace(slot, face), glm::mat4{1.f});
            }
        }
    }

    void ShadowCuller::Submit(uint32_t objectIndex, const bounds::Sphere &sphere, MeshHandle mesh, uint32_t lodCount)
    {
        lodCount = std::max(lodCount, 1u);

        for (uint32_t t{}; t < mPointTargetCount; ++t)
        {
            const auto &target{mPointTargets[t]};
            const float distance{glm::length(sphere.center - target.position)};
            if (distance > target.farPlane + sphere.radius)
                continue;

            const auto lod{std::min(
                lodCount - 1,
                static_cast<uint32_t>(distance / target.farPlane * static_cast<float>(lodCount))
            )};
            const auto command{MakeCommand(mesh, lod, objectIndex)};

            for (uint32_t face{}; face < details::kPointShadowFaceCount; ++face)
            {
                if (!bounds::sphere_in_planes(target.facePlanes[face], sphere))
                    continue;
                const uint32_t list{details::point_shadow_draw_list(target.view, target.slot, face)};
                mPointCommands[list * details::kMaxMeshes + mPointDrawCounts[list]++] = command;
            }
        }
    }

    bool ShadowCuller::Empty() const
    {
        return mPointTargetCount == 0;
    }

    vk::DrawIndexedIndirectCommand ShadowCuller::MakeCommand(MeshHandle mesh, uint32_t lod, uint32_t objectIndex) const
    {
        const auto meshView{mMeshRegistry.View(mesh, lod)};
        return {
            meshView.indexCount,
            1,
            meshView.firstIndex,
            meshView.vertexOffset,
            objectIndex
        };
    }
}
