#include "SceneSerializer.h"

#include <fstream>

#include "AssetPaths.h"

#include "components/entt/AudioSourceComponent.h"
#include "components/entt/BuiltinCamera.h"
#include "components/entt/HierarchyComponent.h"
#include "components/entt/LocalTransform.h"
#include "components/entt/NativeScriptComponent.h"
#include "components/entt/PhysicsComponent.h"
#include "components/entt/TagComponent.h"
#include "components/math/Transform.h"

namespace kailux::scene_serializer
{
    namespace details
    {
        static uint32_t to_id(entt::entity entity)
        {
            return entity == entt::null ? kNoEntity : entt::to_integral(entity);
        }

        template<typename F>
        static void for_each_asset_path(SceneDocument& document, F&& function)
        {
            for (auto &record : document.entities)
            {
                if (record.mesh && !record.mesh->path.empty())
                    function(record.mesh->path);
                if (record.audio && !record.audio->clip.empty())
                    function(record.audio->clip);
            }
        }
    }


    SceneDocument to_document(const Scene &scene)
    {
        SceneDocument document;
        document.version = kSceneVersion;
        document.meta = scene.GetMeta();

        const auto& registry = scene.GetEntityRegistry();
        if (scene.GetSun() != entt::null && registry.all_of<SunData>(scene.GetSun()))
            document.sun = registry.get<SunData>(scene.GetSun());

        if (const auto editor{scene.GetSceneCamera()};
            editor != entt::null && registry.all_of<CameraComponent>(editor))
            document.editorCamera = registry.get<CameraComponent>(editor);

        document.mainCamera = kNoEntity;

        for (auto entity : registry.view<TagComponent>())
        {
            if (entity == scene.GetSun())
                continue;

            if (registry.all_of<BuiltinCamera>(entity))
                continue;

            if (registry.all_of<MeshComponent>(entity) &&
                !registry.all_of<MeshSourceComponent>(entity))
                continue;

            EntityRecord record;
            record.id   = details::to_id(entity);
            record.name = registry.get<TagComponent>(entity).name;

            if (const auto *local = registry.try_get<LocalTransform>(entity))
                record.transform = *local;

            if (const auto *hierarchy = registry.try_get<HierarchyComponent>(entity))
                record.parent = details::to_id(hierarchy->parent);

            if (const auto *source = registry.try_get<MeshSourceComponent>(entity))
                record.mesh = {source->path, source->type};

            if (const auto *material = registry.try_get<MeshMaterialData>(entity))
                record.material = *material;

            if (const auto *light = registry.try_get<PointLightData>(entity))
                record.light = {
                light->positionAndIntensity.w,
                light->range.x,
                light->colorAndEnabled
            };

            if (const auto* camera = registry.try_get<CameraComponent>(entity))
            {
                record.camera = *camera;
                if (camera->isPrimary && document.mainCamera == kNoEntity)
                    document.mainCamera = record.id;
            }

            if (const auto *physics = registry.try_get<PhysicsComponent>(entity))
                record.physics = {
                    physics->type,
                    physics->canBecomeDynamic,
                    physics->mass,
                    physics->isSensor
                };
            
            if (const auto *script = registry.try_get<NativeScriptComponent>(entity); script && !script->name.empty())
                record.script = {script->name, script->params.Raw()};

            if (const auto *audio = registry.try_get<AudioSourceComponent>(entity))
                record.audio = AudioSourceRecord::from(audio->clip, audio->settings);

            document.entities.push_back(std::move(record));
        }

        return document;
    }

    std::string write(const SceneDocument &document, int indent)
    {
        return nlohmann::json(document).dump(indent);
    }

    std::expected<void, Error> write_file(const SceneDocument &document,
        const std::filesystem::path &path, int indent)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
            return std::unexpected(std::format("Could not open '{}' for writing", path.generic_string()));

        auto text{write(document, indent)};
        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!file)
            return std::unexpected(std::format("Failed while writing '{}'", path.generic_string()));

        return {};

    }

    std::expected<SceneDocument, Error> read(std::string_view content)
    {
        nlohmann::json js;
        try
        {
            js = nlohmann::json::parse(content);
        }
        catch (const nlohmann::json::exception &e)
        {
            return std::unexpected(std::format("Scene is not valid json: {}", e.what()));
        }

        Version version;
        version.major = js.value("version_major", 0u);
        version.minor = js.value("version_minor", 0u);
        if (version.major == 0u)
            return std::unexpected("Scene file has no version field");

        if (version.major > kSceneVersion.major ||
            (version.major == kSceneVersion.major && version.minor > kSceneVersion.minor))
            return std::unexpected(std::format(
                "Scene file version {}.{} is newer than supported version {}.{}", version.major, version.minor,
                kSceneVersion.major, kSceneVersion.minor));

        try
        {
            return js.get<SceneDocument>();
        }
        catch (const nlohmann::json::exception &e)
        {
            return std::unexpected(std::format("Scene file is malformed: {}", e.what()));
        }
    }

    std::expected<SceneDocument, Error> read_file(const std::filesystem::path &path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
            return std::unexpected(std::format("Could not open '{}'", path.generic_string()));

        const auto size = file.tellg();
        if (size < 0)
            return std::unexpected(std::format("Could not size '{}'", path.generic_string()));

        std::string content(size, '\0');
        file.seekg(0);
        file.read(content.data(), size);

        return read(content);

    }

    SaveReport store_asset_paths(SceneDocument &document, const std::filesystem::path &scenePath,
        const SaveOptions &options)
    {
        SaveReport report;
        const auto sceneDir{scenePath.parent_path()};

        std::optional<asset_paths::Packer> packer;
        if (options.packAssets)
            packer.emplace(sceneDir, sceneDir / std::format("{}_assets", scenePath.stem().string()),
                           options.workspace);

        details::for_each_asset_path(document, [&](auto& path)
        {
            if (packer)
            {
                if (auto packed{packer->Pack(path)})
                    path = std::move(*packed);
                else
                    report.warnings.push_back(std::move(packed.error()));
            }
            path = asset_paths::to_scene_relative(path, sceneDir);
        });

        if (packer)
            report.packedAssets = packer->GetCopiedCount();
        return report;
    }

    void resolve_asset_paths(SceneDocument &document, const std::filesystem::path &scenePath)
    {
        constexpr Version kSceneRelativeVersion{4, 2};
        const bool        isLegacy{
            document.version.major < kSceneRelativeVersion.major ||
            (document.version.major == kSceneRelativeVersion.major &&
             document.version.minor < kSceneRelativeVersion.minor)
        };

        const auto sceneDir{scenePath.parent_path()};
        details::for_each_asset_path(document, [&](auto& path)
        {
           path = asset_paths::from_scene_relative(path, sceneDir, isLegacy);
        });
    }

    std::expected<SaveReport, Error> save(const Scene &scene,
                                          const std::filesystem::path &path, const SaveOptions &options)
    {
        auto document{to_document(scene)};
        auto report{store_asset_paths(document, path, options)};
        if (const auto result{write_file(document, path)}; !result)
            return std::unexpected(result.error());
        return report;
    }
}