#include "ScriptContext.h"
#include "Engine.h"

#include <cassert>

namespace kailux
{
    ScriptServices::ScriptServices(Engine* engine) : pEngine(engine)
    {
    }

    entt::entity ScriptServices::SpawnPrimitive(Scene &scene, std::string_view name, MeshType type, const Transform &transform) const
    {
        assert(pEngine && "Engine ptr is null!");
        return pEngine->SpawnPrimitive(scene, name, type, transform);
    }

    entt::entity ScriptServices::SpawnMesh(
        Scene &scene,
        const std::filesystem::path &path,
        std::string_view name,
        const Transform & transform,
        std::optional<PhysicsCreationOptions> physics) const
    {
        assert(pEngine && "Engine ptr is null!");
        return pEngine->SpawnMesh(scene, path, name, transform, physics);
    }

    bool ScriptServices::AddBody(Scene &scene, entt::entity entity, PhysicsCreationOptions options) const
    {
        assert(pEngine && "Engine ptr is null!");
        return pEngine->AddPhysicsToEntity(scene, entity, options);
    }

    bool ScriptServices::LoadScene(const std::filesystem::path &path) const
    {
        assert(pEngine && "Engine ptr is null!");
        if (!pEngine->mOnLoadSceneRequest || !pEngine->mOnLoadSceneRequest(path))
            return false;
        pEngine->mStartSimulationOnAttach = true;
        return true;
    }

    void ScriptServices::SetBuiltinCameraControl(bool enable) const
    {
        assert(pEngine && "Engine ptr is null!");
        pEngine->SetBuiltinCameraControl(enable);
    }

    bool ScriptServices::PlayOneShot(const std::filesystem::path &path, std::optional<glm::vec3> position, float gain) const
    {
        assert(pEngine && "Engine ptr is null!");
        return pEngine->mAudioSystem.PlayOneShot(path, position, gain);
    }

    void ScriptServices::SetMasterVolume(float volume) const
    {
        assert(pEngine && "Engine ptr is null!");
        pEngine->mAudioSystem.SetMasterVolume(volume);
    }

    float ScriptServices::GetMasterVolume() const
    {
        assert(pEngine && "Engine ptr is null!");
        return pEngine->mAudioSystem.GetMasterVolume();
    }
}
