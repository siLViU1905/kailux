#include "SceneManager.h"

#include <format>
#include "SceneSerializer.h"
#include "Clock.h"
#include "Log.h"
#include "components/entt/NativeScriptComponent.h"
#include "profiling/Profiling.h"

namespace kailux
{
    SceneManager::SceneManager() : mOnInfoLog([](auto message) { log::file.Info("{}", message); }),
                                   mOnErrorLog([](auto message) { log::file.Error("{}", message); })
    {
    }

    SceneManager::SceneManager(SceneManager &&other) noexcept = default;

    SceneManager &SceneManager::operator=(SceneManager &&other) noexcept = default;

    SceneManager SceneManager::create(std::string_view defaultSceneName)
    {
        SceneManager manager;
        manager.mActive = manager.CreateScene(defaultSceneName);
        return manager;
    }

    void SceneManager::StageNew(std::string_view name)
    {
        mPending = CreateScene(name);
        mPendingRequests.clear();
    }

    SceneManager::Result SceneManager::StageFromFile(const std::filesystem::path &path, const SceneLoadContext &context)
    {
        const auto t{Clock::now()};

        auto document{scene_serializer::read_file(path)};
        if (!document)
        {
            auto error{std::format("Scene '{}' not loaded: {}", path.generic_string(), document.error())};
            mOnErrorLog(error);
            return std::unexpected{std::move(error)};
        }

        scene_serializer::resolve_asset_paths(*document, path);

        auto scene{CreateScene(document->meta.name)};
        auto requests{scene_instantiator::apply(*scene, *document, context)};
        if (!requests)
        {
            auto error{std::format("Scene '{}' not loaded: {}", path.generic_string(), requests.error())};
            mOnErrorLog(error);
            return std::unexpected{std::move(error)};
        }

        scene->SetSavePath(path);
        mPending         = std::move(scene);
        mPendingRequests = std::move(*requests);
        mOnInfoLog(std::format("Scene '{}' staged in {}ms", mPending->GetName(),
                               Clock::get_elapsed<float, TimeType::Milliseconds>(t)));
        return {};
    }

    bool SceneManager::HasPending() const
    {
        return mPending != nullptr;
    }

    std::optional<std::vector<MeshRequest>> SceneManager::CommitPending()
    {
        if (!mPending)
            return std::nullopt;

        mActive = std::move(mPending);
        WatchActive();
        return std::exchange(mPendingRequests, {});
    }

    SceneManager::Result SceneManager::SaveActive(const std::filesystem::path &path)
    {
        auto &scene{GetActiveScene()};
        scene.SetSavePath(path);

        const auto report{scene_serializer::save(scene, path)};
        if (!report)
        {
            auto error{std::format("Scene '{}' not saved: {}", path.generic_string(), report.error())};
            mOnErrorLog(error);
            return std::unexpected{std::move(error)};
        }

        for (const auto &warning : report->warnings)
            mOnErrorLog(std::format("Scene '{}': {}", scene.GetName(), warning));

        auto& registry{scene.GetEntityRegistry()};
        for (const auto entity : registry.view<NativeScriptComponent>())
            registry.get<NativeScriptComponent>(entity).sourceId = entt::to_integral(entity);
        WatchActive();

        mOnInfoLog(std::format("Scene '{}' saved to '{}'", scene.GetName(), path.generic_string()));

        if (report->packedAssets > 0)
            mOnInfoLog(std::format("Copied {} asset(s) to '{}_assets' next to the scene", report->packedAssets,
                                   path.stem().string()));

        return {};
    }

    void SceneManager::ReloadChangedScriptParams()
    {
        KAILUX_PROFILE_CPU(CpuSection::ScriptParamsReload);
        if (!mActiveWatcher.ConsumeChange())
            return;

        const auto& path{mActiveWatcher.GetPath()};
        const auto document{scene_serializer::read_file(path)};
        if (!document)
        {
            mOnErrorLog(std::format("Script params not reloaded from '{}': {}", path.generic_string(),
                                   document.error()));
            return;
        }

        if (const auto changed{scene_instantiator::apply_script_params(GetActiveScene(), *document)}; changed > 0)
            mOnInfoLog(std::format("Reloaded script params for {} entit{} from '{}'", changed,
                                   changed == 1 ? "y" : "ies", path.generic_string()));
    }

    void SceneManager::SetOnInfoLog(OnLog &&callback)
    {
        mOnInfoLog = std::move(callback);
    }

    void SceneManager::SetOnErrorLog(OnLog &&callback)
    {
        mOnErrorLog = std::move(callback);
    }

    SceneManager::~SceneManager() = default;

    Scoped<Scene> SceneManager::CreateScene(std::string_view name)
    {
        return create_scoped<Scene>(Scene::create(mNextSceneId++, name));
    }

    void SceneManager::WatchActive()
    {
        const auto& savePath{GetActiveScene().GetSavePath()};
        if (savePath.empty())
        {
            mActiveWatcher.Stop();
            return;
        }

        std::error_code error;
        if (mActiveWatcher.IsWatching() && std::filesystem::absolute(savePath, error) == mActiveWatcher.GetPath())
            return;

        if (!mActiveWatcher.Watch(savePath))
            mOnErrorLog(std::format("Could not watch '{}' for script param changes", savePath.generic_string()));
    }
}
