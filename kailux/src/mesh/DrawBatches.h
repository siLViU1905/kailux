#pragma once
#include <span>
#include <vector>

#include "MeshRegistry.h"

namespace kailux
{
    class DrawBatches
    {
    public:
        struct Batch
        {
            MeshHandle meshHandle;
            uint32_t   objectCount{};
            uint32_t   firstObject{};

            bool operator==(const Batch& other) const;
        };

        void Begin();

        uint32_t Add(MeshHandle mesh);

        bool End(uint64_t meshGeneration);

        void WriteCommands(std::span<vk::DrawIndexedIndirectCommand> commands, const MeshRegistry& meshRegistry) const;

        std::span<const Batch> GetBatches() const;
        uint32_t               GetCommandCount() const;
        uint32_t               GetObjectCount() const;

        static uint32_t command_index(uint32_t batch, uint32_t lod);

    private:
        std::vector<Batch>    mBatches;
        std::vector<Batch>    mPreviousBatches;
        std::vector<uint32_t> mBatchOfMesh; 
        uint32_t              mObjectCount{};
        uint64_t              mMeshGeneration{~0ull};
    };
}