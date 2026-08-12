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

        scene.setMeta(document.meta);

        if (scene.getSun() != entt::null)
            scene.getEntityRegistry().get<SunData>(scene.getSun()) = document.sun;

        std::unordered_map<uint32_t, entt::entity> remap;
        remap.reserve(document.entities.size());

        std::vector<MeshRequest> requests;
        requests.reserve(meshCount);

        for (const auto &record : document.entities)
        {
            const auto entity = scene.createEntity(record.name);

            if (record.transform)
                scene.setLocalTransform(entity, *record.transform);

            if (record.id != kNoEntity)
                remap.emplace(record.id, entity);

            const GizmoRegistry& gizmoRegistry = context.gizmoRegistry;
            if (record.light)
            {
                GizmoComponent gizmo{
                    gizmoRegistry.getBuiltins().pointLight,
                    0.5f,
                    glm::vec4(glm::vec3(record.light->colorAndEnabled), 1.f)
                };
                scene.attachPointLight(entity, gizmo, *record.light);
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
                scene.attachCamera(entity, *record.camera, context.windowWidth, context.windowHeight);
        }

        if (const auto it{remap.find(document.mainCamera)}; it != remap.end())
            scene.setMainCamera(it->second);
        else
        {
            log::console.warning("No camera found in scene '{}'. Falling back...", scene.getName());
            const auto fallback = scene.createCameraEntity(
                "MainCamera", true, context.windowWidth, context.windowHeight);
            scene.setMainCamera(fallback);
        }

        for (const auto& record : document.entities)
        {
            if (record.parent == kNoEntity)
                continue;

            const auto child = remap.find(record.id);
            const auto parent = remap.find(record.parent);
            if (child == remap.end() || parent == remap.end())
                continue;

            scene.setParent(child->second, parent->second);
        }

        scene.update();
        return requests;
    }
}
