#pragma once
#include "DirectionalShadowSet.h"
#include "PointShadowSet.h"
#include "FrameData.h"
#include "mesh/MeshRegistry.h"
#include "utilities/Bounds.h"

namespace kailux
{
    class ShadowCuller
    {
    public:
        ShadowCuller(
            FrameData &frame,
            const MeshRegistry &meshRegistry,
            std::span<const DirectionalShadowSet, details::kMaxCameraViews> directionalSets,
            std::span<const PointShadowSet, details::kMaxCameraViews> pointSets
        );

        void Submit(uint32_t objectIndex, const bounds::Sphere &sphere, MeshHandle mesh, uint32_t lodCount);

        bool Empty() const;

    private:
        struct CascadeTarget
        {
            uint32_t                 view;
            uint32_t                 cascade;
            std::array<glm::vec4, 6> planes;
        };

        struct PointTarget
        {
            uint32_t  view;
            uint32_t  slot;
            glm::vec3 position;
            float     farPlane;
            std::array<std::array<glm::vec4, 6>, details::kPointShadowFaceCount> facePlanes;
        };

        vk::DrawIndexedIndirectCommand MakeCommand(MeshHandle mesh, uint32_t lod, uint32_t objectIndex) const;

        const MeshRegistry &mMeshRegistry;

        std::span<vk::DrawIndexedIndirectCommand> mCascadeCommands;
        std::span<vk::DrawIndexedIndirectCommand> mPointCommands;

        std::array<uint32_t, details::kDirectionalShadowDrawLists> &mCascadeDrawCounts;
        std::array<uint32_t, details::kPointShadowDrawLists>       &mPointDrawCounts;

        std::array<CascadeTarget, details::kDirectionalShadowDrawLists>               mCascadeTargets{};
        uint32_t                                                                      mCascadeTargetCount{};
        std::array<PointTarget, details::kMaxCameraViews * details::kMaxPointShadows> mPointTargets{};
        uint32_t                                                                      mPointTargetCount{};
    };
}