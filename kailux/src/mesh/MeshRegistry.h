#pragma once
#include <cstdint>
#include <deque>
#include <list>

#include "MeshGeometry.h"
#include "Vertex.h"
#include "Core.h"
#include "buffer/Buffer.h"
#include "buffer/BufferAllocator.h"
#include "utilities/LinearZone.h"
#include "utilities/FreeListZone.h"

namespace kailux
{
    using MeshHandle = Handle;

    struct MeshView
    {
        uint32_t firstIndex{};
        uint32_t indexCount{};
        int32_t vertexOffset{};
    };

    struct LodInfo
    {
        uint32_t count{1};
        std::array<float, details::kMaxGeometryLods - 1> errors{};
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
        void                  Destroy(MeshHandle handle);
        MeshView              View(MeshHandle handle, uint32_t lod = 0) const;
        void                  Bind(vk::CommandBuffer cmd) const;
        uint32_t              GetMeshCount() const;

        LodInfo GetLodInfo(MeshHandle handle) const;

        BuiltinMeshes GetBuiltins() const;

        uint64_t GetGeneration() const;

        MeshBufferRegions GetRegions(MeshHandle handle) const;

    private:
        struct LodAlloc
        {
            uint32_t firstIndex{};
            uint32_t indexCount{};
            float    error{};
        };
        struct MeshAlloc
        {
            vk::DeviceSize vertexOffset;
            uint32_t vertexCount;
            vk::DeviceSize indexOffset;
            uint32_t indexCount;
            std::array<LodAlloc, details::kMaxGeometryLods> lods{};
            uint32_t lodCount{1};
            bool is_builtin{};
        };

        MeshHandle AllocSlot();
        MeshHandle UploadInternal(std::span<const Vertex> vertices,
                                  std::span<const IndexType> indices,
                                  const Context &context,
                                  vk::CommandBuffer cmd,
                                  std::vector<Buffer> &stagingBuffers,
                                  bool isBuiltin,
                                  std::span<const mesh_geometry::LodLevel> extraLods);

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

        uint64_t mGeneration{};

    public:
        MeshHandle Upload(const Context &context,
                               vk::CommandBuffer cmd,
                               const mesh_geometry::MeshData &data,
                               std::vector<Buffer> &stagingBuffer);
    };
}
