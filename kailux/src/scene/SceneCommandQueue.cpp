#include "SceneCommandQueue.h"

#include "Application.h"
#include "components/entt/HierarchyComponent.h"
#include "components/entt/LocalTransform.h"
#include "components/entt/MeshSourceComponent.h"
#include "components/entt/PhysicsComponent.h"
#include "components/entt/TagComponent.h"

namespace kailux
{
    namespace scene_cmd_queue
    {
        bool can_delete_entity(const Scene &scene, entt::entity entity)
        {
            return scene.GetEntityRegistry().valid(entity) &&
                   entity != scene.GetSun() &&
                   entity != scene.GetSceneCamera();
        }

        bool name_exists(const entt::registry &registry, entt::entity self, std::string_view name)
        {
            for (const auto other : registry.view<TagComponent>())
                if (other != self && registry.get<TagComponent>(other).name == name)
                    return true;
            return false;
        }

        void propagate_rename_to_children(entt::registry &registry, entt::entity entity, std::string_view oldName)
        {
            const auto *hierarchy{registry.try_get<HierarchyComponent>(entity)};
            if (!hierarchy || hierarchy->parent != entt::null)
                return;

            const auto &newName{registry.get<TagComponent>(entity).name};
            for (const auto child : hierarchy->children)
            {
                auto &name{registry.get<TagComponent>(child).name};
                if (name.substr(0, oldName.size()) == oldName)
                    name.replace(0, oldName.size(), newName);
            }
        }

        void SceneCommandExecutor::operator()(scene_commands::SpawnPrimitive &command) const
        {
            engine.GetPendingMeshDataQueue().Emplace(
                    entt::null,
                    "",
                    mesh_loader::LoadData{},
                    "",
                    Transform{},
                    MeshMaterialData{},
                    command.type
                );
        }

        void SceneCommandExecutor::operator()(scene_commands::ImportMesh &command) const
        {
            engine.HandleMeshDragDrop(command.path, application.GetThreadDispatcher());
        }

        void SceneCommandExecutor::operator()(scene_commands::SpawnLight &command) const
        {
            engine.AddLightEntity(scene, command.type);
        }

        void SceneCommandExecutor::operator()(scene_commands::SpawnCamera &command) const
        {
            engine.AddCameraEntity(scene, command.extent.x, command.extent.y);
        }

        void SceneCommandExecutor::operator()(scene_commands::SpawnAudioSource &) const
        {
            engine.AddAudioSourceEntity(scene);
        }

        void SceneCommandExecutor::operator()(scene_commands::DeleteEntity &command) const
        {
            if (!can_delete_entity(scene, command.entity))
                return;

            if (const auto *source{Registry().try_get<MeshSourceComponent>(command.entity)};
                source && source->type == MeshType::Loaded)
                engine.UnregisterMesh(scene, command.entity);

            scene.DestroyEntity(command.entity);
        }

        void SceneCommandExecutor::operator()(scene_commands::RenameEntity &command) const
        {
            auto &registry{Registry()};
            if (!registry.valid(command.entity) || command.name.empty())
                return;

            if (name_exists(registry, command.entity, command.name))
                return;

            auto &name{registry.get<TagComponent>(command.entity).name};
            const auto oldName{std::exchange(name, std::move(command.name))};
            propagate_rename_to_children(registry, command.entity, oldName);
        }

        void SceneCommandExecutor::operator()(scene_commands::SetTransform &command) const
        {
            auto &registry{Registry()};
            if (!registry.valid(command.entity) || !registry.all_of<LocalTransform>(command.entity))
                return;

            scene.SetLocalTransform(command.entity, command.transform);
            if (command.syncBodyScale)
                if (const auto *physics{registry.try_get<PhysicsComponent>(command.entity)})
                    engine.UpdateBodyScale(physics->handle, command.transform.scale);
        }

        void SceneCommandExecutor::operator()(scene_commands::AddPhysics &command) const
        {
            if (!Registry().valid(command.entity))
                return;

            engine.AddPhysicsToEntity(scene, command.entity, command.options);
        }

        void SceneCommandExecutor::operator()(scene_commands::AddAudioSource &command) const
        {
            if (!Registry().valid(command.entity))
                return;

            engine.AddAudioSourceToEntity(scene, command.entity);
        }

        void SceneCommandExecutor::operator()(scene_commands::RemoveAudioSource &command) const
        {
            scene.DetachAudioSource(command.entity);
        }

        void SceneCommandExecutor::operator()(scene_commands::SetBodyType &command) const
        {
            auto &registry{Registry()};
            if (!registry.valid(command.entity))
                return;

            auto *physics{registry.try_get<PhysicsComponent>(command.entity)};
            if (!physics)
                return;

            physics->type = command.type;
            engine.UpdateBodyType(physics->handle, command.type);
        }

        void SceneCommandExecutor::operator()(scene_commands::SetBodyMass &command) const
        {
            auto &registry{Registry()};
            if (!registry.valid(command.entity))
                return;

            auto *physics{registry.try_get<PhysicsComponent>(command.entity)};
            if (!physics)
                return;

            physics->mass = command.mass;
            if (command.commit)
                engine.UpdateBodyMass(physics->handle, command.mass);
        }

        void SceneCommandExecutor::operator()(scene_commands::LoadScene &command) const
        {
            if (application.LoadScene(command.path) && command.onStaged)
                command.onStaged();
        }
    }


    void SceneCommandQueue::Submit(SceneCommand &&command)
    {
        mPending.push_back(std::move(command));
    }

    void SceneCommandQueue::Flush(Application &application, Scene &scene)
    {
        if (mPending.empty())
            return;

        std::swap(mPending, mExecuting);

        const scene_cmd_queue::SceneCommandExecutor executor{application, scene, application.GetEngine()};
        for (auto& command : mExecuting)
            std::visit(executor, command);

        mExecuting.clear();
    }

    void SceneCommandQueue::Clear()
    {
        mPending.clear();
    }

    bool SceneCommandQueue::Empty() const
    {
        return mPending.empty();
    }
}
