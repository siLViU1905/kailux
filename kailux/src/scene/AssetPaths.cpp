#include "AssetPaths.h"

#include <format>

namespace kailux::asset_paths
{
    namespace fs = std::filesystem;

    namespace details
    {
        static fs::path absolute_normal(const fs::path &path)
        {
            std::error_code ec;
            const auto absolute{fs::absolute(path, ec)};
            return (ec ? path : absolute).lexically_normal();
        }

        static bool is_inside(const fs::path &path, const fs::path &dir)
        {
            const auto relative{path.lexically_relative(dir)};
            return !relative.empty() && *relative.begin() != "..";
        }
    }

    std::string to_scene_relative(const std::filesystem::path &path, const std::filesystem::path &sceneDir)
    {
        if (path.empty())
            return {};

        const auto absolute{details::absolute_normal(path)};
        const auto relative{absolute.lexically_relative(details::absolute_normal(sceneDir))};

        return relative.empty() ? absolute.generic_string() : relative.generic_string();
    }

    std::string from_scene_relative(const std::filesystem::path &stored, const std::filesystem::path &sceneDir, bool legacy)
    {
        if (stored.empty() || stored.is_absolute())
            return {stored};

        const auto resolved{(sceneDir / stored).lexically_normal()};
        if (legacy)
        {
            std::error_code ec;
            if (fs::exists(stored, ec) || !fs::exists(resolved, ec))
                return std::string{stored};
        }

        return resolved.generic_string();
    }

    Packer::Packer(const fs::path &sceneDir, const fs::path &packDir, const fs::path &workspace)
     : mSceneDir(details::absolute_normal(sceneDir)),
       mPackDir(details::absolute_normal(packDir)),
       mWorkspace(details::absolute_normal(workspace))
    {
    }

    std::expected<std::string, Error> Packer::Pack(const std::filesystem::path &path)
    {
        if (path.empty())
            return std::string{};

        const auto absolute{details::absolute_normal(path)};

        std::error_code ec;
        if (!fs::exists(absolute, ec))
            return std::unexpected(std::format("Asset '{}' does not exist", path.generic_string()));

        if (details::is_inside(absolute, mSceneDir))
            return std::string{path};

        fs::path sourceRoot, rest;
        if (details::is_inside(absolute, mWorkspace))
        {
            const auto relative{absolute.lexically_relative(mWorkspace)};
            sourceRoot = mWorkspace / *relative.begin();
            for (auto it{std::next(relative.begin())}; it != relative.end(); ++it)
                rest /= *it;
        }
        else
            sourceRoot = absolute;

        auto found{mCopied.find(sourceRoot.generic_string())};
        if (found == mCopied.end())
        {
            const auto destinationRoot{Claim(sourceRoot, mPackDir / sourceRoot.filename())};

            fs::create_directories(destinationRoot.parent_path(), ec);
            if (ec)
                return std::unexpected(std::format("Could not create '{}': {}",
                                                   destinationRoot.parent_path().generic_string(), ec.message()));

            fs::copy(sourceRoot, destinationRoot,
                     fs::copy_options::recursive | fs::copy_options::update_existing, ec);
            if (ec)
                return std::unexpected(std::format("Could not copy '{}' to '{}': {}", sourceRoot.generic_string(),
                                                   destinationRoot.generic_string(), ec.message()));

            found = mCopied.emplace(sourceRoot.generic_string(), destinationRoot).first;
        }
        return (rest.empty() ? found->second : found->second / rest).generic_string();
    }

    uint32_t Packer::GetCopiedCount() const
    {
        return static_cast<uint32_t>(mCopied.size());
    }

    std::filesystem::path Packer::Claim(const std::filesystem::path &sourceRoot, const std::filesystem::path &wanted)
    {
        auto candidate{wanted};
        for (uint32_t i{1};; ++i)
        {
            const auto [it, inserted]{mClaimed.emplace(candidate.generic_string(), sourceRoot.generic_string())};
            if (inserted || it->second == sourceRoot.generic_string())
                return candidate;

            candidate = wanted.parent_path() /
                std::format("{}_{}{}", wanted.stem().string(), i, wanted.extension().string());
        }
    }
}
