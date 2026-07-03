#include "MeshLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glm/gtc/type_ptr.hpp>
#include <magic_enum/magic_enum.hpp>

#include "core/Geometry.h"

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

        LoadData loadData;
        for (uint32_t i = 0; i < scene->mNumMaterials; i++)
        {
            MaterialPaths paths;
            extract_material_paths(scene->mMaterials[i], paths, meshDirectoryPath);
            loadData.materials.push_back(process_material_paths(paths));
        }

        process_node(scene->mRootNode, scene, kParentMatrix, loadData, meshDirectoryPath);

        return loadData;
    }

    glm::mat4 MeshLoader::aiMatrix4x4_to_glm(const aiMatrix4x4 &m)
    {
        return glm::transpose(glm::make_mat4(&m.a1));
    }

    void MeshLoader::process_node(const aiNode *node,
                                  const aiScene *scene,
                                  const glm::mat4 &parentMatrix,
                                  MeshLoader::LoadData &outLoadData,
                                  std::string_view directoryPath
    )
    {
        auto worldMatrix = parentMatrix * aiMatrix4x4_to_glm(node->mTransformation);

        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            const aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            SubMeshData submesh;
            submesh.name = mesh->mName.C_Str();
            submesh.localTransform = worldMatrix;
            submesh.materialIndex = mesh->mMaterialIndex;

            process_mesh(mesh, submesh.meshData);

            submesh.boundingSphere = Geometry::computeBoundingSphere(submesh.meshData.vertices);

            outLoadData.submeshes.push_back(std::move(submesh));
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++)
            process_node(node->mChildren[i], scene, worldMatrix, outLoadData, directoryPath);
    }

    void MeshLoader::process_mesh(const aiMesh *mesh,
                                  MeshRegistry::MeshData &outMeshData)
    {
        outMeshData.vertices.reserve(mesh->mNumVertices);

        for (uint32_t i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex v{};

            v.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            if (mesh->HasNormals())
                v.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

            if (mesh->mTextureCoords[0])
                v.uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            else
                v.uv = glm::vec2(0.f, 0.f);

            if (mesh->HasTangentsAndBitangents())
                v.tangent = glm::vec4(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 1.0f);

            outMeshData.vertices.push_back(v);
        }

        outMeshData.indices.reserve(mesh->mNumFaces * 3);
        for (uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
            const aiFace &face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
                outMeshData.indices.push_back(face.mIndices[j]);
        }
    }

    void MeshLoader::extract_material_paths(const aiMaterial *material, MaterialPaths &outPaths,
        std::string_view directoryPath)
    {
        auto getMaterialMemberPtr = [](TextureType type, MaterialPaths& paths) -> std::string* {
            switch (type) {
                case TextureType::Albedo:    return &paths.albedoPath;
                case TextureType::Normal:    return &paths.normalPath;
                case TextureType::Roughness: return &paths.roughnessPath;
                case TextureType::Metallic:  return &paths.metallicPath;
                case TextureType::AO:        return &paths.aoPath;
                default:                     return nullptr;
            }
        };

        std::ranges::for_each(TextureRegistry::kTextureTypes, [&](auto type)
        {
            auto aiType = static_cast<aiTextureType>(type);
            auto& targetPath = *getMaterialMemberPtr(type, outPaths);

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

    TextureRegistry::MaterialData MeshLoader::process_material_paths(const MaterialPaths &paths)
    {
        TextureRegistry::MaterialData data;
        auto imgData = ImageLoader::load_image(paths.albedoPath);
        if (imgData)
            data.albedoData = std::move(*imgData);
        imgData = ImageLoader::load_image(paths.normalPath);
        if (imgData)
            data.normalData = std::move(*imgData);
        imgData = ImageLoader::load_image(paths.roughnessPath);
        if (imgData)
            data.roughnessData = std::move(*imgData);
        imgData = ImageLoader::load_image(paths.metallicPath);
        if (imgData)
            data.metallicData = std::move(*imgData);
        imgData = ImageLoader::load_image(paths.aoPath);
        if (imgData)
            data.aoData = std::move(*imgData);
        return data;
    }
}
