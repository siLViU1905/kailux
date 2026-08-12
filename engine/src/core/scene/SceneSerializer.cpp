#include "SceneSerializer.h"

#include <fstream>

#include "core/components/entt/HierarchyComponent.h"
#include "core/components/entt/PhysicsComponent.h"
#include "core/components/entt/TagComponent.h"
#include "core/components/gpu/TransformComponent.h"

namespace kailux
{
    namespace details
    {
        static uint32_t to_id(entt::entity entity)
        {
            return entity == entt::null ? kNoEntity : entt::to_integral(entity);
        }
    }


    SceneDocument SceneSerializer::to_document(const Scene &scene)
    {
        SceneDocument document;
        document.version = kSceneVersion;
        document.meta = scene.getMeta();

        const auto& registry = scene.getEntityRegistry();
        if (scene.getSun() != entt::null && registry.all_of<SunData>(scene.getSun()))
            document.sun = registry.get<SunData>(scene.getSun());
 
        document.mainCamera = details::to_id(scene.getMainCamera());
 
        for (auto entity : registry.view<TagComponent>())
        {
            if (entity == scene.getSun())
                continue;

            if (registry.all_of<MeshComponent>(entity) &&
                !registry.all_of<MeshSourceComponent>(entity))
                continue;
 
            EntityRecord record;
            record.id   = details::to_id(entity);
            record.name = registry.get<TagComponent>(entity).name;
 
            if (const auto *transform = registry.try_get<TransformComponent>(entity))
                record.transform = transform->transform;
 
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
                record.camera = *camera;
 
            if (const auto *physics = registry.try_get<PhysicsComponent>(entity))
                record.physics = {physics->type, physics->canBecomeDynamic};
 
            document.entities.push_back(std::move(record));
        }
 
        return document;
    }

    std::string SceneSerializer::write(const SceneDocument &document, int indent)
    {
        return nlohmann::json(document).dump(indent);
    }

    std::expected<void, SceneSerializer::Error> SceneSerializer::writeFile(const SceneDocument &document,
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

    std::expected<SceneDocument, SceneSerializer::Error> SceneSerializer::read(std::string_view content)
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

    std::expected<SceneDocument, SceneSerializer::Error> SceneSerializer::read_file(const std::filesystem::path &path)
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

    std::expected<void, SceneSerializer::Error> SceneSerializer::save(const Scene &scene,
        const std::filesystem::path &path)
    {
        return writeFile(to_document(scene), path);
    }
}
