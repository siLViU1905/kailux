#include "SceneManager.h"

#include <format>
#include "SceneSerializer.h"
#include "Clock.h"
#include "Log.h"

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

        mOnInfoLog(std::format("Scene '{}' saved to '{}'", scene.GetName(), path.generic_string()));

        if (report->packedAssets > 0)
            mOnInfoLog(std::format("Copied {} asset(s) to '{}_assets' next to the scene", report->packedAssets,
                                   path.stem().string()));

        return {};
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
}
