#include "SceneInstantiator.h"

#include "Scene.h"
#include "Log.h"
#include "components/entt/AudioSourceComponent.h"
#include "components/entt/NativeScriptComponent.h"

namespace kailux::scene_instantiator
{
    std::expected<std::vector<MeshRequest>,Error>apply(Scene &scene,
                                                       const SceneDocument &document, const SceneLoadContext &context)
    {
        size_t meshCount{}, lightCount{}, cameraCount{};
        for (const auto &record : document.entities)
        {
            meshCount   += record.mesh ? 1 : 0;
            lightCount  += record.light ? 1 : 0;
            cameraCount += record.camera ? 1 : 0;
        }
        if (meshCount > details::kMaxMeshes)
            return std::unexpected(std::format(
                "Scene contains {} meshes, limit is {}", meshCount, details::kMaxMeshes));
        if (lightCount > details::kMaxPointLights)
            return std::unexpected(std::format(
                "Scene contains {} point lights, limit is {}", lightCount, details::kMaxPointLights));
        if (cameraCount > details::kMaxCameras)
            return std::unexpected(std::format(
                "Scene contains {} cameras, limit is {}", cameraCount, details::kMaxCameras));

        scene.SetMeta(document.meta);

        auto& registry = scene.GetEntityRegistry();

        if (scene.GetSun() != entt::null)
            registry.get<SunData>(scene.GetSun()) = document.sun;

        if (document.editorCamera)
            if (const auto editor{scene.GetSceneCamera()};
                editor != entt::null && registry.all_of<CameraComponent>(editor))
            {
                auto& camera = registry.get<CameraComponent>(editor);
                camera       = *document.editorCamera;
                camera.isPrimary = false;
            }

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
                    record.transform.value_or(Transform{}),
                    record.material.value_or(MeshMaterialData{}),
                    record.physics
                );

            if (record.camera)
            {
                if (!record.transform)
                    scene.SetLocalTransform(entity, {record.camera->position});

                const GizmoComponent gizmo{
                    gizmoRegistry.GetBuiltins().camera,
                    0.5f,
                    {1.f, 1.f, 1.f, 1.f}
                };
                scene.AttachCamera(entity, gizmo, *record.camera);
            }

            if (record.script)
            {
                auto& script{registry.emplace_or_replace<NativeScriptComponent>(entity)};
                script.params = ScriptParams{record.script->params};
                if (!script.Bind(record.script->name))
                    log::console.Warning("Script '{}' on '{}' is not registered, it will not run",
                                         record.script->name, record.name);
            }

            if (record.audio)
            {
                const GizmoComponent gizmo{
                    gizmoRegistry.GetBuiltins().audioSource,
                    0.5f,
                    {1.f, 1.f, 1.f, 1.f}
                };
                scene.AttachAudioSource(
                    entity,
                    gizmo,
                    {
                        record.audio->clip,
                        record.audio->ToSettings()
                    }
                );
            }
        }

        constexpr Version kPrimaryCameraVersion{2, 1};
        const bool        isLegacy{
            document.version.major < kPrimaryCameraVersion.major ||
            (document.version.major == kPrimaryCameraVersion.major &&
             document.version.minor < kPrimaryCameraVersion.minor)
        };

        entt::entity primary{entt::null};
        if (!isLegacy)
            if (const auto it{remap.find(document.mainCamera)};
                it != remap.end() && registry.all_of<CameraComponent>(it->second))
                primary = it->second;

        if (primary == entt::null)
            primary = scene.GetPrimaryCamera();

        if (primary != entt::null)
            scene.SetPrimaryCamera(primary);
        else
            log::console.Warning(
                "Scene '{}' has no camera. Add one before starting the simulation.", scene.GetName());

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
        scene.UpdateCameras();
        return requests;
    }
}
