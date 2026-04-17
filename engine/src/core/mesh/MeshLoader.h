#pragma once
#include <expected>

#include "MeshRegistry.h"
#include <assimp/scene.h>

#include "core/texture/TextureRegistry.h"

namespace kailux
{


    class MeshLoader
    {
    public:
        struct LoadData
        {
            MeshRegistry::MeshData        meshData;
            TextureRegistry::MaterialData materialData;
        };
        using LoadResult = std::expected<LoadData, std::string>;
        static LoadResult load(std::string_view path);

    private:
        static constexpr float        s_ScaleFactor = 0.1f;
        inline static const glm::mat4 s_ParentMatrix = glm::scale(glm::mat4(1.f), glm::vec3(s_ScaleFactor));

        static glm::mat4 aiMatrix4x4_to_glm(const aiMatrix4x4 &m);

        struct MaterialPaths
        {
            std::string albedoPath;
            std::string normalPath;
            std::string roughnessPath;
            std::string metallicPath;
            std::string aoPath;
        };

        static void process_node(const aiNode *node,
                                 const aiScene *scene,
                                 const glm::mat4 &parentMatrix,
                                 MeshRegistry::MeshData &outMeshData,
                                 MaterialPaths &outPaths,
                                 std::string_view directoryPath
        );
        static void process_mesh(const aiMesh *mesh,
                                 const aiScene *scene,
                                 const glm::mat4 &worldMatrix,
                                 MeshRegistry::MeshData &outMeshData,
                                 MaterialPaths &outPaths,
                                 std::string_view directoryPath
        );
        static void process_mesh_material(const aiMesh *mesh,
                                          const aiScene *scene,
                                          MaterialPaths &outPaths,
                                          std::string_view directoryPath
        );
        static TextureRegistry::MaterialData process_material_paths(const MaterialPaths& paths);
    };
}
