#include "DrawBatches.h"

namespace kailux
{
    bool DrawBatches::Batch::operator==(const Batch& other) const
    {
        return meshHandle.index == other.meshHandle.index
               && objectCount == other.objectCount;
    }

    void DrawBatches::Begin()
    {
        for (const auto& batch : mBatches)
            mBatchOfMesh[batch.meshHandle.index] = Handle::kInvalidIndex;

        std::swap(mBatches, mPreviousBatches);
        mBatches.clear();
        mObjectCount = 0;
    }

    uint32_t DrawBatches::Add(MeshHandle mesh)
    {
        assert(mesh.Valid());

        if (mesh.index >= mBatchOfMesh.size())
            mBatchOfMesh.resize(mesh.index + 1, Handle::kInvalidIndex);

        auto& batchIndex{mBatchOfMesh[mesh.index]};
        if (batchIndex == Handle::kInvalidIndex)
        {
            batchIndex = static_cast<uint32_t>(mBatches.size());
            mBatches.emplace_back(mesh);
        }

        ++mBatches[batchIndex].objectCount;
        ++mObjectCount;
        return batchIndex;
    }

    bool DrawBatches::End(uint64_t meshGeneration)
    {
        uint32_t firstObject{};
        for (auto& batch : mBatches)
        {
            batch.firstObject = firstObject;
            firstObject += batch.objectCount;
        }

        const bool changed{
            mMeshGeneration != meshGeneration
            || mBatches != mPreviousBatches
        };
        mMeshGeneration = meshGeneration;
        return changed;
    }

    void DrawBatches::WriteCommands(std::span<vk::DrawIndexedIndirectCommand> commands,
        const MeshRegistry &meshRegistry) const
    {
        for (uint32_t batchIndex{}; batchIndex < mBatches.size(); ++batchIndex)
        {
            const auto& batch{mBatches[batchIndex]};
            for (uint32_t lod{}; lod < details::kMaxGeometryLods; ++lod)
            {
                const auto meshView{meshRegistry.View(batch.meshHandle, lod)};
                commands[command_index(batchIndex, lod)] = vk::DrawIndexedIndirectCommand{
                    meshView.indexCount,
                    0,
                    meshView.firstIndex,
                    meshView.vertexOffset,
                    batch.firstObject * details::kMaxGeometryLods + lod * batch.objectCount
                };
            }
        }
    }

    std::span<const DrawBatches::Batch> DrawBatches::GetBatches() const
    {
        return mBatches;
    }

    uint32_t DrawBatches::GetCommandCount() const
    {
        return static_cast<uint32_t>(mBatches.size())  * details::kMaxGeometryLods;
    }

    uint32_t DrawBatches::GetObjectCount() const
    {
        return mObjectCount;
    }

    uint32_t DrawBatches::command_index(uint32_t batch, uint32_t lod)
    {
        return batch * details::kMaxGeometryLods + lod;
    }
}
