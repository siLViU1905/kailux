#pragma once
#include <cstdint>
#include <deque>
#include <list>

#include "Vertex.h"
#include "core/Core.h"
#include "core/buffer/Buffer.h"
#include "core/buffer/BufferAllocator.h"
#include "core/utilities/LinearZone.h"

namespace kailux
{
    using MeshHandle = Handle;

    struct MeshView
    {
        uint32_t firstIndex{};
        uint32_t indexCount{};
        int32_t vertexOffset{};
    };

    struct MeshBufferRegions
    {
        vk::Buffer     vertexBuffer;
        vk::DeviceSize vertexOffset{};
        vk::DeviceSize vertexSize{};
        vk::Buffer     indexBuffer;
        vk::DeviceSize indexOffset{};
        vk::DeviceSize indexSize{};
    };

    struct BuiltinMeshes
    {
        MeshHandle cube;
        MeshHandle sphere;
    };

    class MeshRegistry
    {
    public:
        static constexpr vk::DeviceSize kVertexAlignment  = sizeof(Vertex);
        static constexpr vk::DeviceSize kBuiltinZoneSize  = (16 * 1024 * 1024 / kVertexAlignment) * kVertexAlignment;
        static constexpr vk::DeviceSize kAssetZoneSize    = (1000 * 1024 * 1024 / kVertexAlignment) * kVertexAlignment;
        static constexpr vk::DeviceSize kTotalSize        = kBuiltinZoneSize + kAssetZoneSize;

        using IndexType = uint32_t;

        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(MeshRegistry)

        static MeshRegistry  create(const Context &context,
                                   vk::CommandBuffer cmd,
                                   std::vector<Buffer> &stagingBuffers);
        void                  destroy(MeshHandle handle);
        MeshView              view(MeshHandle handle) const;
        void                  bind(vk::CommandBuffer cmd) const;
        uint32_t              getMeshCount() const;

        BuiltinMeshes getBuiltins() const;

        MeshBufferRegions getRegions(MeshHandle handle) const;

    private:
        struct Block
        {
            vk::DeviceSize offset{};
            vk::DeviceSize size{};
        };

        struct FreeListZone
        {
            static constexpr vk::DeviceSize kDefaultAlignment = 16;

            vk::DeviceSize base{};
            vk::DeviceSize capacity{};
            std::list<Block> freeBlocks;
            std::list<Block> usedBlocks;

            vk::DeviceSize alloc(vk::DeviceSize size, vk::DeviceSize alignment = kDefaultAlignment);

            void free(vk::DeviceSize offset);
        };

        struct MeshAlloc
        {
            vk::DeviceSize vertexOffset;
            uint32_t vertexCount;
            vk::DeviceSize indexOffset;
            uint32_t indexCount;
            bool is_builtin{};
        };

    private:
        MeshHandle allocSlot();
        MeshHandle uploadInternal(std::span<const Vertex> vertices,
                                  std::span<const IndexType> indices,
                                  const Context &context,
                                  vk::CommandBuffer cmd,
                                  std::vector<Buffer> &stagingBuffers,
                                  bool isBuiltin);

        static void upload_buffer_region(const void *data,
                                         vk::DeviceSize size,
                                         Buffer &dst,
                                         vk::DeviceSize dstOffset,
                                         const Context &context,
                                         vk::CommandBuffer cmd,
                                         std::vector<Buffer> &stagingBuffers);

        Buffer mVertexBuffer;
        Buffer mIndexBuffer;

        LinearZone mBuiltinVertexZone;
        LinearZone mBuiltinIndexZone;
        FreeListZone mAssetVertexZone;
        FreeListZone mAssetIndexZone;

        std::vector<MeshAlloc> mAllocs;
        std::vector<uint32_t>  mFreeSlots;

        BuiltinMeshes mBuiltins;

    public:
        struct MeshData
        {
            std::vector<Vertex> vertices;
            std::vector<IndexType> indices;
        };

        static MeshData generate_cube();
        static MeshData generate_sphere(uint32_t sectors = 32, uint32_t stacks = 32);

        MeshHandle      upload(const Context &context,
                               vk::CommandBuffer cmd,
                               const MeshData &data,
                               std::vector<Buffer> &stagingBuffer);
    };
}
