#pragma once

#include <vector>

#include "Scene.h"
#include "SceneCommands.h"

namespace kailux
{
    class Application;
    class Scene;
    class Engine;

    namespace scene_cmd_queue
    {
        bool can_delete_entity(const Scene &scene, entt::entity entity);
        bool name_exists(const entt::registry &registry, entt::entity self, std::string_view name);
        void propagate_rename_to_children(entt::registry &registry, entt::entity entity, std::string_view oldName);

        struct SceneCommandExecutor
        {
            Application &application;
            Scene       &scene;
            Engine      &engine;

            decltype(auto) Registry(this auto&& self)
            {
                return (self.scene.GetEntityRegistry());
            }

            void operator()(scene_commands::SpawnPrimitive &command) const;
            void operator()(scene_commands::ImportMesh &command) const;
            void operator()(scene_commands::SpawnLight &command) const;
            void operator()(scene_commands::SpawnCamera &command) const;
            void operator()(scene_commands::SpawnAudioSource &) const;
            void operator()(scene_commands::DeleteEntity &command) const;
            void operator()(scene_commands::RenameEntity &command) const;
            void operator()(scene_commands::SetTransform &command) const;
            void operator()(scene_commands::AddPhysics &command) const;
            void operator()(scene_commands::AddAudioSource &command) const;
            void operator()(scene_commands::RemoveAudioSource &command) const;
            void operator()(scene_commands::SetBodyType &command) const;
            void operator()(scene_commands::SetBodyMass &command) const;
            void operator()(scene_commands::LoadScene &command) const;
        };
    }

    class SceneCommandQueue
    {
    public:
        void Submit(SceneCommand&& command);

        void Flush(Application& application, Scene& scene);

        void Clear();

        bool Empty() const;

    private:
        std::vector<SceneCommand> mPending;
        std::vector<SceneCommand> mExecuting;
    };
}