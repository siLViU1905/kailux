#pragma once
#include "Scene.h"
#include "SceneDocument.h"

namespace kailux::scene_serializer
{
    using Error = std::string;

    SceneDocument to_document(const Scene &scene);

    std::string write(const SceneDocument &document, int indent = 3);

    std::expected<void, Error> write_file(const SceneDocument &document,
                                          const std::filesystem::path &path,
                                          int indent = 3);

    std::expected<SceneDocument, Error> read(std::string_view content);

    std::expected<SceneDocument, Error> read_file(const std::filesystem::path &path);

    std::expected<void, Error> save(const Scene &scene, const std::filesystem::path &path);
}
