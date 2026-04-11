#pragma once
#include <expected>

#include "MeshRegistry.h"
#include <assimp/scene.h>

namespace kailux
{
    class MeshLoader
    {
    public:
        using LoadResult = std::expected<MeshRegistry::MeshData, std::string>;
        static LoadResult load(std::string_view path);

    private:
        static constexpr float        s_ScaleFactor = 0.001f;
        inline static const glm::mat4 s_ParentMatrix = glm::scale(glm::mat4(1.f), glm::vec3(s_ScaleFactor));

        static glm::mat4 aiMatrix4x4_to_glm(const aiMatrix4x4 &m);

        static void process_node(const aiNode *node,
                                 const aiScene *scene,
                                 const glm::mat4 &parentMatrix,
                                 MeshRegistry::MeshData &out
        );
        static void process_mesh(const aiMesh *mesh,
                                 const glm::mat4 &worldMatrix,
                                 MeshRegistry::MeshData &out
        );
    };
}
