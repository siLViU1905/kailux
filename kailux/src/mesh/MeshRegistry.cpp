#include "MeshRegistry.h"
#include <numbers>

namespace kailux
{
    MeshRegistry::MeshRegistry()
    {
    }

    MeshRegistry::MeshRegistry(MeshRegistry &&other) noexcept : mVertexBuffer(std::move(other.mVertexBuffer)),
                                                                mIndexBuffer(std::move(other.mIndexBuffer)),
                                                                mBuiltinVertexZone(other.mBuiltinVertexZone),
                                                                mBuiltinIndexZone(other.mBuiltinIndexZone),
                                                                mAssetVertexZone(std::move(other.mAssetVertexZone)),
                                                                mAssetIndexZone(std::move(other.mAssetIndexZone)),
                                                                mAllocs(std::move(other.mAllocs)),
                                                                mFreeSlots(std::move(other.mFreeSlots)),
                                                                mBuiltins(other.mBuiltins)
    {
    }

    MeshRegistry &MeshRegistry::operator=(MeshRegistry &&other) noexcept
    {
        if (this != &other)
        {
            mVertexBuffer = std::move(other.mVertexBuffer);
            mIndexBuffer = std::move(other.mIndexBuffer);
            mBuiltinVertexZone = other.mBuiltinVertexZone;
            mBuiltinIndexZone = other.mBuiltinIndexZone;
            mAssetVertexZone = std::move(other.mAssetVertexZone);
            mAssetIndexZone = std::move(other.mAssetIndexZone);
            mAllocs = std::move(other.mAllocs);
            mFreeSlots = std::move(other.mFreeSlots);
            mBuiltins = other.mBuiltins;
        }
        return *this;
    }

    MeshRegistry MeshRegistry::create(const Context &context, vk::CommandBuffer cmd,
                                      std::vector<Buffer> &stagingBuffers)
    {
        MeshRegistry registry;
        registry.mVertexBuffer = buffer_allocator::alloc_vertex(context, kTotalSize);
        registry.mIndexBuffer = buffer_allocator::alloc_index(context, kTotalSize / 2);

        registry.mBuiltinVertexZone = {0, kBuiltinZoneSize, 0};
        registry.mBuiltinIndexZone = {0, kBuiltinZoneSize / 2, 0};
        registry.mAssetVertexZone = FreeListZone::create(kBuiltinZoneSize, kAssetZoneSize);
        registry.mAssetIndexZone = FreeListZone::create(kBuiltinZoneSize / 2, kAssetZoneSize / 2);

        auto uploadShape = [&](auto genFn, MeshHandle &out)
        {
            auto data = genFn();
            out = registry.UploadInternal(data.vertices, data.indices, context, cmd, stagingBuffers, true, {});
        };

        uploadShape([]() { return mesh_geometry::generate_cube(); }, registry.mBuiltins.cube);
        uploadShape([]() { return mesh_geometry::generate_sphere(); }, registry.mBuiltins.sphere);

        return registry;
    }

    void MeshRegistry::Destroy(MeshHandle handle)
    {
        assert(handle.Valid());
        auto &a = mAllocs[handle.index];
        assert(!a.is_builtin && "Cannot destroy built-in shapes");

        mAssetVertexZone.Free(a.vertexOffset);
        mAssetIndexZone.Free(a.indexOffset);
        mFreeSlots.push_back(handle.index);
        a = {};
    }

    MeshView MeshRegistry::View(MeshHandle handle, uint32_t lod) const
    {
        assert(handle.Valid());
        const auto &alloc{mAllocs[handle.index]};
        const auto &l{alloc.lods[std::min(lod, alloc.lodCount - 1)]};
        return {
            static_cast<uint32_t>(alloc.indexOffset / sizeof(IndexType)) + l.firstIndex,
        l.indexCount,
        static_cast<int32_t>(alloc.vertexOffset / sizeof(Vertex))
        };
    }

    void MeshRegistry::Bind(vk::CommandBuffer cmd) const
    {
        cmd.bindVertexBuffers(0, mVertexBuffer.GetBuffer(), {0});
        cmd.bindIndexBuffer(mIndexBuffer.GetBuffer(), 0, vk::IndexType::eUint32);
    }

    uint32_t MeshRegistry::GetMeshCount() const
    {
        return static_cast<uint32_t>(mAllocs.size());
    }

    LodInfo MeshRegistry::GetLodInfo(MeshHandle handle) const
    {
        assert(handle.Valid());
        const auto &alloc = mAllocs[handle.index];

        LodInfo info;
        info.count = alloc.lodCount;

        for (uint32_t i{1}; i < alloc.lodCount; ++i)
            info.errors[i - 1] = alloc.lods[i].error;

        return info;
    }

    BuiltinMeshes MeshRegistry::GetBuiltins() const
    {
        return mBuiltins;
    }

    MeshBufferRegions MeshRegistry::GetRegions(MeshHandle handle) const
    {
        assert(handle.Valid());
        const auto &alloc = mAllocs[handle.index];
        return {
            mVertexBuffer.GetBuffer(),
            alloc.vertexOffset,
            static_cast<vk::DeviceSize>(alloc.vertexCount) * sizeof(Vertex),
            mIndexBuffer.GetBuffer(),
            alloc.indexOffset,
            static_cast<vk::DeviceSize>(alloc.indexCount) * sizeof(IndexType)
        };
    }

    MeshHandle MeshRegistry::AllocSlot()
    {
        if (!mFreeSlots.empty())
        {
            auto id = mFreeSlots.back();
            mFreeSlots.pop_back();
            return {id};
        }
        mAllocs.emplace_back();

        return {static_cast<uint32_t>(mAllocs.size() - 1)};
    }

    MeshHandle MeshRegistry::UploadInternal(
        std::span<const Vertex> vertices,
        std::span<const IndexType> indices,
        const Context &context,
        vk::CommandBuffer cmd,
        std::vector<Buffer> &stagingBuffers,
        bool isBuiltin,
        std::span<const mesh_geometry::LodLevel> extraLods)
    {
        auto totalIndices{indices.size()};
        for (const auto &lod : extraLods)
            totalIndices += lod.indices.size();

        std::vector<IndexType> packed;
        packed.reserve(totalIndices);
        packed.assign(indices.begin(), indices.end());

        std::array<LodAlloc, details::kMaxGeometryLods> lods{};
        lods.front() = {
            0,
            static_cast<uint32_t>(indices.size()),
            0.f
        };

        uint32_t lodCount{1};
        for (const auto &lod : extraLods)
        {
            lods[lodCount] = {
                static_cast<uint32_t>(packed.size()),
                static_cast<uint32_t>(lod.indices.size()),
                lod.error
            };
            packed.insert(packed.end(), lod.indices.begin(), lod.indices.end());
            ++lodCount;
        }

        const vk::DeviceSize vsize{vertices.size_bytes()};
        const vk::DeviceSize isize{packed.size() * sizeof(IndexType)};

        vk::DeviceSize voffset, ioffset;

        if (isBuiltin)
        {
            voffset = mBuiltinVertexZone.Alloc(vsize, sizeof(Vertex));
            ioffset = mBuiltinIndexZone.Alloc(isize, sizeof(IndexType));
        } else
        {
            voffset = mAssetVertexZone.Alloc(vsize, sizeof(Vertex));
            ioffset = mAssetIndexZone.Alloc(isize, sizeof(IndexType));
        }

        upload_buffer_region(vertices.data(), vsize, mVertexBuffer, voffset, context, cmd, stagingBuffers);
        upload_buffer_region(packed.data(), isize, mIndexBuffer, ioffset, context, cmd, stagingBuffers);

        auto handle = AllocSlot();
        mAllocs[handle.index] = {
            voffset,
            static_cast<uint32_t>(vertices.size()),
            ioffset,
            static_cast<uint32_t>(packed.size()),
            lods,
            lodCount,
            isBuiltin
        };
        return handle;
    }

    void MeshRegistry::upload_buffer_region(const void *data, vk::DeviceSize size, Buffer &dst,
                                            vk::DeviceSize dstOffset, const Context &context, vk::CommandBuffer cmd,
                                            std::vector<Buffer> &stagingBuffers)
    {
        auto &staging = stagingBuffers.emplace_back(buffer_allocator::alloc_staging(context, size));
        staging.Upload(data, size);

        vk::BufferCopy region(0, dstOffset, size);
        cmd.copyBuffer(staging.GetBuffer(), dst.GetBuffer(), region);
    }

    MeshHandle MeshRegistry::Upload(const Context &context,
                                    vk::CommandBuffer cmd,
                                    const mesh_geometry::MeshData &data,
                                    std::vector<Buffer> &stagingBuffer
    )
    {
        return UploadInternal(
            data.vertices,
            data.indices,
            context,
            cmd,
            stagingBuffer,
            false,
            data.lods
        );
    }
}
