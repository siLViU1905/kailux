#pragma once
#include "Scene.h"
#include "SceneDocument.h"

namespace kailux::scene_serializer
{
    using Error = std::string;

    struct SaveOptions
    {
        bool                  packAssets{true};
        std::filesystem::path workspace{kailux::details::kWorkspaceDefaultPath};
    };

    struct SaveReport
    {
        uint32_t           packedAssets{};
        std::vector<Error> warnings;
    };

    SceneDocument to_document(const Scene &scene);

    std::string write(const SceneDocument &document, int indent = 3);

    std::expected<void, Error> write_file(const SceneDocument &document,
                                          const std::filesystem::path &path,
                                          int indent = 3);

    std::expected<SceneDocument, Error> read(std::string_view content);

    std::expected<SceneDocument, Error> read_file(const std::filesystem::path &path);

    SaveReport store_asset_paths(SceneDocument& document, const std::filesystem::path& scenePath, const SaveOptions& options);
    void       resolve_asset_paths(SceneDocument& document, const std::filesystem::path& scenePath);

    std::expected<SaveReport, Error> save(const Scene &scene, const std::filesystem::path &path, const SaveOptions &options = {});
}
