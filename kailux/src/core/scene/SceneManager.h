#pragma once

#include <expected>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "Scene.h"
#include "SceneInstantiator.h"
#include "core/Core.h"

namespace kailux
{
    class SceneManager
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(SceneManager)

        static constexpr std::string_view kSceneFileExtension{"klx"};

        static SceneManager create(std::string_view defaultSceneName = "MainScene");

        decltype(auto) GetActiveScene(this auto&& self)
        {
            return (*self.mActive);
        }

        using Error  = std::string;
        using Result = std::expected<void, Error>;
        void   StageNew(std::string_view name);
        Result StageFromFile(const std::filesystem::path &path, const SceneLoadContext &context);
        bool HasPending() const;

        std::optional<std::vector<MeshRequest>> CommitPending();

        Result SaveActive(const std::filesystem::path &path);

        using OnLog = std::move_only_function<void(std::string_view)>;
        void SetOnInfoLog(OnLog &&callback);
        void SetOnErrorLog(OnLog &&callback);

        ~SceneManager();

    private:
        Scoped<Scene> CreateScene(std::string_view name);

        Scoped<Scene>            mActive;
        Scoped<Scene>            mPending;
        std::vector<MeshRequest> mPendingRequests;

        SceneId mNextSceneId{kInvalidSceneId + 1};

        OnLog mOnInfoLog;
        OnLog mOnErrorLog;
    };
}