#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace kailux::asset_paths
{
    using Error = std::string;

    std::string to_scene_relative(const std::filesystem::path &path, const std::filesystem::path &sceneDir);
    std::string from_scene_relative(const std::filesystem::path &stored, const std::filesystem::path &sceneDir, bool legacy);

    class Packer
    {
    public:
        Packer(const std::filesystem::path &sceneDir,
               const std::filesystem::path &packDir,
               const std::filesystem::path &workspace);

        std::expected<std::string, Error> Pack(const std::filesystem::path &path);

        uint32_t GetCopiedCount() const;

    private:
        std::filesystem::path Claim(const std::filesystem::path &sourceRoot, const std::filesystem::path &wanted);

        std::filesystem::path mSceneDir;
        std::filesystem::path mPackDir;
        std::filesystem::path mWorkspace;

        std::unordered_map<std::string, std::filesystem::path> mCopied;
        std::unordered_map<std::string, std::string>           mClaimed;
    };
}
