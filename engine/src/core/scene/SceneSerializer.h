#pragma once
#include "Scene.h"
#include "SceneDocument.h"

namespace kailux
{
    class SceneSerializer
    {
    public:
        using Error = std::string;

        static SceneDocument to_document(const Scene &scene);
        static std::string   write(const SceneDocument &document, int indent = 3);
        static std::expected<void, Error> writeFile(const SceneDocument &document,
                                                    const std::filesystem::path &path,
                                                    int indent = 3);

        static std::expected<SceneDocument, Error> read(std::string_view content);
        static std::expected<SceneDocument, Error> read_file(const std::filesystem::path &path);

        static std::expected<void, Error> save(const Scene &scene, const std::filesystem::path &path);
    };
}
