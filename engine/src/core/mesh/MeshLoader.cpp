#include "MeshLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glm/gtc/type_ptr.hpp>
#include <magic_enum/magic_enum.hpp>

namespace kailux
{
    MeshLoader::LoadResult MeshLoader::load(std::string_view path)
    {
        Assimp::Importer importer;

        const aiScene *scene = importer.ReadFile(path.data(),
                                                 aiProcess_Triangulate |
                                                 aiProcess_GenSmoothNormals |
                                                 aiProcess_CalcTangentSpace |
                                                 aiProcess_FlipUVs |
                                                 aiProcess_JoinIdenticalVertices
        );

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            return std::unexpected(std::format("Assimp failed to load '{}': {}", path, importer.GetErrorString()));

        auto meshDirectoryPath = path.substr(0, path.find_last_of('/'));
        LoadData outData;
        process_node(scene->mRootNode, scene, s_ParentMatrix, outData, meshDirectoryPath);
        return outData;
    }

    glm::mat4 MeshLoader::aiMatrix4x4_to_glm(const aiMatrix4x4 &m)
    {
        return glm::transpose(glm::make_mat4(&m.a1));
    }

    void MeshLoader::process_node(const aiNode *node,
                                  const aiScene *scene,
                                  const glm::mat4 &parentMatrix,
                                  LoadData &outData, std::string_view directoryPath
    )
    {
        auto worldMatrix = parentMatrix * aiMatrix4x4_to_glm(node->mTransformation);

        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            const aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            process_mesh(mesh, scene, worldMatrix, outData, directoryPath);
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++)
            process_node(node->mChildren[i], scene, worldMatrix, outData, directoryPath);
    }

    void MeshLoader::process_mesh(const aiMesh *mesh, const aiScene *scene, const glm::mat4 &worldMatrix,
                                  LoadData &outData, std::string_view directoryPath)
    {
        auto normalMatrix = glm::mat3(glm::transpose(glm::inverse(worldMatrix)));

        auto vertexOffset = static_cast<uint32_t>(outData.meshData.vertices.size());
        outData.meshData.vertices.reserve(outData.meshData.vertices.size() + mesh->mNumVertices);

        for (uint32_t i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex v;

            glm::vec3 pos = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
            v.position = glm::vec3(worldMatrix * glm::vec4(pos, 1.0f));

            if (mesh->HasNormals())
            {
                glm::vec3 norm = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
                v.normal = glm::normalize(normalMatrix * norm);
            }

            if (mesh->mTextureCoords[0])
                v.uv = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};

            if (mesh->HasTangentsAndBitangents())
            {
                glm::vec3 t = {mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z};
                glm::vec3 b = {mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z};
                glm::vec3 n = v.normal;

                glm::vec3 tangent = glm::normalize(normalMatrix * t);

                tangent = glm::normalize(tangent - glm::dot(tangent, n) * n);
                float handedness = (glm::dot(glm::cross(n, tangent), glm::normalize(normalMatrix * b)) < 0.f)
                                       ? -1.f
                                       : 1.f;

                v.tangent = glm::vec4(tangent, handedness);
            }

            outData.meshData.vertices.push_back(v);
        }

        outData.meshData.indices.reserve(outData.meshData.indices.size() + mesh->mNumFaces * 3);
        for (uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
            const aiFace &face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
                outData.meshData.indices.push_back(face.mIndices[j] + vertexOffset);
        }
        process_mesh_material(mesh, scene, outData, directoryPath);
    }

    void MeshLoader::process_mesh_material(const aiMesh *mesh, const aiScene *scene, LoadData &outData, std::string_view directoryPath)
    {
        if (mesh->mMaterialIndex >= scene->mNumMaterials)
            return;

        constexpr auto textureTypes = magic_enum::enum_values<TextureType>();
        const auto *material = scene->mMaterials[mesh->mMaterialIndex];
        auto getMaterialMemberPtr = [](TextureType type, TextureRegistry::MaterialData& data) -> std::string* {
            switch (type) {
                case TextureType::Albedo:    return &data.albedoPath;
                case TextureType::Normal:    return &data.normalPath;
                case TextureType::Roughness: return &data.roughnessPath;
                case TextureType::Metallic:  return &data.metallicPath;
                case TextureType::AO:        return &data.aoPath;
                default:                     return nullptr;
            }
        };

        std::ranges::for_each(textureTypes, [&](auto type)
        {
            auto aiType = static_cast<aiTextureType>(type);
            auto& targetPath = *getMaterialMemberPtr(type, outData.materialData);

            if (material->GetTextureCount(aiType) &&
                targetPath.empty()
            )
            {
                aiString path;
                if (material->GetTexture(aiType, 0, &path) == aiReturn_SUCCESS)
                {
                    targetPath = path.C_Str();
                    targetPath = std::string(directoryPath) + "/" + targetPath;
                }
            }
        });
    }
}
