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
        registry.mVertexBuffer = BufferAllocator::alloc_vertex(context, kTotalSize);
        registry.mIndexBuffer = BufferAllocator::alloc_index(context, kTotalSize / 2);

        registry.mBuiltinVertexZone = {0, kBuiltinZoneSize, 0};
        registry.mBuiltinIndexZone = {0, kBuiltinZoneSize / 2, 0};
        registry.mAssetVertexZone = {kBuiltinZoneSize, kAssetZoneSize};
        registry.mAssetIndexZone = {kBuiltinZoneSize / 2, kAssetZoneSize / 2};

        registry.mAssetVertexZone.freeBlocks.emplace_back(kBuiltinZoneSize, kAssetZoneSize);
        registry.mAssetIndexZone.freeBlocks.emplace_back(kBuiltinZoneSize / 2, kAssetZoneSize / 2);

        auto uploadShape = [&](auto genFn, MeshHandle &out)
        {
            auto data = genFn();
            out = registry.UploadInternal(data.vertices, data.indices, context, cmd, stagingBuffers, true);
        };

        uploadShape([]() { return MeshGeometry::generate_cube(); }, registry.mBuiltins.cube);
        uploadShape([]() { return MeshGeometry::generate_sphere(); }, registry.mBuiltins.sphere);

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

    MeshView MeshRegistry::View(MeshHandle handle) const
    {
        assert(handle.Valid());
        const auto &alloc = mAllocs[handle.index];
        return {
            static_cast<uint32_t>(alloc.indexOffset / sizeof(IndexType)),
            alloc.indexCount,
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

    vk::DeviceSize MeshRegistry::FreeListZone::Alloc(vk::DeviceSize size, vk::DeviceSize alignment)
    {
        for (auto it = freeBlocks.begin(); it != freeBlocks.end(); ++it)
        {
            vk::DeviceSize aligned = ((it->offset + alignment - 1) / alignment) * alignment;
            vk::DeviceSize padding = aligned - it->offset;

            if (it->size >= size + padding)
            {
                if (aligned < base || (aligned + size) > (base + capacity))
                    return 0;

                vk::DeviceSize offset = aligned;

                vk::DeviceSize remaining = it->size - size - padding;
                if (remaining > 64)
                {
                    Block leftover(aligned + size, remaining);
                    freeBlocks.insert(std::next(it), leftover);
                }

                if (padding > 0)
                    it->size = padding;
                else
                    freeBlocks.erase(it);

                usedBlocks.emplace_back(offset, size);
                return offset;
            }
        }
        throw std::runtime_error("Asset zone out of memory");
    }

    void MeshRegistry::FreeListZone::Free(vk::DeviceSize offset)
    {
        auto it = std::ranges::find_if(usedBlocks, [offset](const Block &b)
        {
            return b.offset == offset;
        });

        if (it == usedBlocks.end())
            throw std::runtime_error("MeshRegistry::free — invalid offset");

        Block freed = *it;
        usedBlocks.erase(it);

        auto pos = std::ranges::lower_bound(freeBlocks, freed.offset,
                                            {}, &Block::offset);
        auto inserted = freeBlocks.insert(pos, freed);

        auto next = std::next(inserted);
        if (next != freeBlocks.end() &&
            inserted->offset + inserted->size == next->offset)
        {
            inserted->size += next->size;
            freeBlocks.erase(next);
        }

        if (inserted != freeBlocks.begin())
        {
            auto prev = std::prev(inserted);
            if (prev->offset + prev->size == inserted->offset)
            {
                prev->size += inserted->size;
                freeBlocks.erase(inserted);
            }
        }
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

    MeshHandle MeshRegistry::UploadInternal(std::span<const Vertex> vertices, std::span<const IndexType> indices,
                                            const Context &context, vk::CommandBuffer cmd,
                                            std::vector<Buffer> &stagingBuffers, bool isBuiltin)
    {
        vk::DeviceSize vsize = vertices.size_bytes();
        vk::DeviceSize isize = indices.size_bytes();

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
        upload_buffer_region(indices.data(), isize, mIndexBuffer, ioffset, context, cmd, stagingBuffers);

        auto handle = AllocSlot();
        mAllocs[handle.index] = {
            voffset,
            static_cast<uint32_t>(vertices.size()),
            ioffset,
            static_cast<uint32_t>(indices.size()),
            isBuiltin
        };
        return handle;
    }

    void MeshRegistry::upload_buffer_region(const void *data, vk::DeviceSize size, Buffer &dst,
                                            vk::DeviceSize dstOffset, const Context &context, vk::CommandBuffer cmd,
                                            std::vector<Buffer> &stagingBuffers)
    {
        auto &staging = stagingBuffers.emplace_back(BufferAllocator::alloc_staging(context, size));
        staging.Upload(data, size);

        vk::BufferCopy region(0, dstOffset, size);
        cmd.copyBuffer(staging.GetBuffer(), dst.GetBuffer(), region);
    }

    MeshHandle MeshRegistry::Upload(const Context &context,
                                    vk::CommandBuffer cmd, const MeshGeometry::MeshData &data, std::vector<Buffer> &stagingBuffer)
    {
        return UploadInternal(data.vertices, data.indices, context, cmd, stagingBuffer, false);
    }
}
