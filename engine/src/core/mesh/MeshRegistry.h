#pragma once
#include <cstdint>
#include <deque>
#include <list>

#include "Vertex.h"
#include "core/Core.h"
#include "core/buffer/Buffer.h"
#include "core/buffer/BufferAllocator.h"

namespace kailux
{
    struct MeshHandle
    {
        static constexpr uint32_t s_InvalidIndex = ~0u;

        uint32_t index = s_InvalidIndex;

        constexpr bool valid() const { return index != s_InvalidIndex; }
    };

    struct MeshView
    {
        uint32_t firstIndex{};
        uint32_t indexCount{};
        int32_t vertexOffset{};
    };

    struct BuiltinMeshes
    {
        MeshHandle cube;
        MeshHandle sphere;
    };

    class MeshRegistry
    {
    public:
        static constexpr vk::DeviceSize s_BuiltinZoneSize = 16 * 1024 * 1024; //16MB
        static constexpr vk::DeviceSize s_AssetZoneSize = 64 * 1024 * 1024; //64MB
        static constexpr vk::DeviceSize s_TotalSize = s_BuiltinZoneSize + s_AssetZoneSize;

        using IndexType = uint32_t;

    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(MeshRegistry)

        static MeshRegistry create(const Context &context,
                                   vk::CommandBuffer cmd,
                                   std::vector<Buffer> &stagingBuffers);

        MeshHandle           upload(std::span<const Vertex> vertices,
                                    std::span<const IndexType> indices,
                                    const Context &context,
                                    vk::CommandBuffer cmd,
                                    std::vector<Buffer> &stagingBuffer);
        void                  destroy(MeshHandle handle);
        MeshView              view(MeshHandle handle) const;
        std::vector<MeshView> viewAll() const;
        void                  bind(vk::CommandBuffer cmd) const;
        uint32_t              getMeshCount() const;

        BuiltinMeshes getBuiltins() const;

    private:
        struct LinearZone
        {
            static constexpr vk::DeviceSize s_DefaultAlignment = 16;

            vk::DeviceSize base{};
            vk::DeviceSize capacity{};
            vk::DeviceSize cursor{};

            vk::DeviceSize alloc(vk::DeviceSize size, vk::DeviceSize alignment = s_DefaultAlignment);
        };

        struct Block
        {
            vk::DeviceSize offset{};
            vk::DeviceSize size{};
        };

        struct FreeListZone
        {
            static constexpr vk::DeviceSize s_DefaultAlignment = 16;

            vk::DeviceSize base{};
            vk::DeviceSize capacity{};
            std::list<Block> freeBlocks;
            std::list<Block> usedBlocks;

            vk::DeviceSize alloc(vk::DeviceSize size, vk::DeviceSize alignment = s_DefaultAlignment);

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

        Buffer m_VertexBuffer;
        Buffer m_IndexBuffer;

        LinearZone m_BuiltinVertexZone;
        LinearZone m_BuiltinIndexZone;
        FreeListZone m_AssetVertexZone;
        FreeListZone m_AssetIndexZone;

        std::vector<MeshAlloc> m_Allocs;
        std::deque<uint32_t> m_FreeSlots;

        BuiltinMeshes m_Builtins;

    public:
        struct MeshData
        {
            std::vector<Vertex> vertices;
            std::vector<IndexType> indices;
        };

        static MeshData generate_cube();
        static MeshData generate_sphere(uint32_t sectors = 32, uint32_t stacks = 32);
    };
}
