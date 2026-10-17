#pragma once
#include <expected>

#include "MeshGeometry.h"
#include <assimp/scene.h>

#include "texture/TextureRegistry.h"

namespace kailux::mesh_loader
{
    struct SubMeshData
    {
        std::string                   name;
        mesh_geometry::MeshData       meshData;
        glm::vec4                     boundingSphere{};
        uint32_t                      materialIndex{};
        glm::mat4                     localTransform{1.0f};
    };

    struct LoadData
    {
        std::vector<SubMeshData> submeshes;
        std::vector<TextureRegistry::MaterialData> materials;
    };
    using LoadResult = std::expected<LoadData, std::string>;
    LoadResult load(const std::filesystem::path &path);

    namespace details
    {
        constexpr float        kScaleFactor = 0.1f;
        inline const glm::mat4 kParentMatrix = glm::scale(glm::mat4(1.f), glm::vec3(kScaleFactor));

        glm::mat4 ai_matrix4x4_to_glm(const aiMatrix4x4 &m);

        struct MaterialPaths
        {
            std::string albedoPath;
            std::string normalPath;
            std::string roughnessPath;
            std::string metallicPath;
            std::string aoPath;
        };

        void process_materials(
            const aiScene *scene,
            LoadData &outLoadData,
            const std::filesystem::path &directoryPath
        );
        void process_node(const aiNode *node,
                                 const aiScene *scene,
                                 const glm::mat4 &parentMatrix,
                                 LoadData &outLoadData
        );
        void process_mesh(const aiMesh *mesh,
                                 mesh_geometry::MeshData &outMeshData
        );
        void extract_material_paths(const aiMaterial *material,
                                           MaterialPaths &outPaths,
                                           const std::filesystem::path &directoryPath);
        TextureRegistry::MaterialData process_material_paths(const MaterialPaths& paths);
    }
}
