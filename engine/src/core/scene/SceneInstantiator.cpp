#include "SceneInstantiator.h"

#include "Scene.h"
#include "core/Log.h"

namespace kailux
{
    std::expected<std::vector<MeshRequest>, SceneInstantiator::Error> SceneInstantiator::apply(Scene &scene,
        const SceneDocument &document, const SceneLoadContext &context)
    {
        size_t meshCount{}, lightCount{};
        for (const auto &record : document.entities)
        {
            meshCount  += record.mesh ? 1 : 0;
            lightCount += record.light ? 1 : 0;
        }
        if (meshCount > details::kMaxMeshes)
            return std::unexpected(std::format(
                "Scene contains {} meshes, limit is {}", meshCount, details::kMaxMeshes));
        if (lightCount > details::kMaxPointLights)
            return std::unexpected(std::format(
                "Scene contains {} point lights, limit is {}", lightCount, details::kMaxPointLights));

        scene.SetMeta(document.meta);

        if (scene.GetSun() != entt::null)
            scene.GetEntityRegistry().get<SunData>(scene.GetSun()) = document.sun;

        std::unordered_map<uint32_t, entt::entity> remap;
        remap.reserve(document.entities.size());

        std::vector<MeshRequest> requests;
        requests.reserve(meshCount);

        for (const auto &record : document.entities)
        {
            const auto entity = scene.CreateEntity(record.name);

            if (record.transform)
                scene.SetLocalTransform(entity, *record.transform);

            if (record.id != kNoEntity)
                remap.emplace(record.id, entity);

            const GizmoRegistry& gizmoRegistry = context.gizmoRegistry;
            if (record.light)
            {
                GizmoComponent gizmo{
                    gizmoRegistry.GetBuiltins().pointLight,
                    0.5f,
                    glm::vec4(glm::vec3(record.light->colorAndEnabled), 1.f)
                };
                scene.AttachPointLight(entity, gizmo, *record.light);
            }

            if (record.mesh)
                requests.emplace_back(
                    entity,
                    record.name,
                    *record.mesh,
                    record.transform.value_or(MeshTransformData{}),
                    record.material.value_or(MeshMaterialData{}),
                    record.physics
                );

            if (record.camera)
            {
                GizmoComponent gizmo{
                    gizmoRegistry.GetBuiltins().camera,
                    0.5f,
                    {}
                };
                scene.AttachCamera(entity, gizmo, *record.camera);
        }
        }

        if (const auto it{remap.find(document.mainCamera)}; it != remap.end())
            scene.SetMainCamera(it->second);
        else
        {
            log::console.Warning("No camera found in scene '{}'. Falling back...", scene.GetName());
            const auto fallback = scene.CreateBuiltinCameraEntity("MainCamera");
            scene.SetMainCamera(fallback);
        }

        for (const auto& record : document.entities)
        {
            if (record.parent == kNoEntity)
                continue;

            const auto child = remap.find(record.id);
            const auto parent = remap.find(record.parent);
            if (child == remap.end() || parent == remap.end())
                continue;

            scene.SetParent(child->second, parent->second);
        }

        scene.Update();
        return requests;
    }
}
