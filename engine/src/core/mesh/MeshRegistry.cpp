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
            out = registry.uploadInternal(data.vertices, data.indices, context, cmd, stagingBuffers, true);
        };

        uploadShape([]() { return generate_cube(); }, registry.mBuiltins.cube);
        uploadShape([]() { return generate_sphere(); }, registry.mBuiltins.sphere);

        return registry;
    }

    void MeshRegistry::destroy(MeshHandle handle)
    {
        assert(handle.valid());
        auto &a = mAllocs[handle.index];
        assert(!a.is_builtin && "Cannot destroy built-in shapes");

        mAssetVertexZone.free(a.vertexOffset);
        mAssetIndexZone.free(a.indexOffset);
        mFreeSlots.push_back(handle.index);
        a = {};
    }

    MeshView MeshRegistry::view(MeshHandle handle) const
    {
        assert(handle.valid());
        const auto &alloc = mAllocs[handle.index];
        return {
            static_cast<uint32_t>(alloc.indexOffset / sizeof(IndexType)),
            alloc.indexCount,
            static_cast<int32_t>(alloc.vertexOffset / sizeof(Vertex))
        };
    }

    void MeshRegistry::bind(vk::CommandBuffer cmd) const
    {
        cmd.bindVertexBuffers(0, mVertexBuffer.getBuffer(), {0});
        cmd.bindIndexBuffer(mIndexBuffer.getBuffer(), 0, vk::IndexType::eUint32);
    }

    uint32_t MeshRegistry::getMeshCount() const
    {
        return static_cast<uint32_t>(mAllocs.size());
    }

    BuiltinMeshes MeshRegistry::getBuiltins() const
    {
        return mBuiltins;
    }

    MeshBufferRegions MeshRegistry::getRegions(MeshHandle handle) const
    {
        assert(handle.valid());
        const auto &alloc = mAllocs[handle.index];
        return {
            mVertexBuffer.getBuffer(),
            alloc.vertexOffset,
            static_cast<vk::DeviceSize>(alloc.vertexCount) * sizeof(Vertex),
            mIndexBuffer.getBuffer(),
            alloc.indexOffset,
            static_cast<vk::DeviceSize>(alloc.indexCount) * sizeof(IndexType)
        };
    }

    vk::DeviceSize MeshRegistry::FreeListZone::alloc(vk::DeviceSize size, vk::DeviceSize alignment)
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

    void MeshRegistry::FreeListZone::free(vk::DeviceSize offset)
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

    MeshHandle MeshRegistry::allocSlot()
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

    MeshHandle MeshRegistry::uploadInternal(std::span<const Vertex> vertices, std::span<const IndexType> indices,
                                            const Context &context, vk::CommandBuffer cmd,
                                            std::vector<Buffer> &stagingBuffers, bool isBuiltin)
    {
        vk::DeviceSize vsize = vertices.size_bytes();
        vk::DeviceSize isize = indices.size_bytes();

        vk::DeviceSize voffset, ioffset;

        if (isBuiltin)
        {
            voffset = mBuiltinVertexZone.alloc(vsize, sizeof(Vertex));
            ioffset = mBuiltinIndexZone.alloc(isize, sizeof(IndexType));
        } else
        {
            voffset = mAssetVertexZone.alloc(vsize, sizeof(Vertex));
            ioffset = mAssetIndexZone.alloc(isize, sizeof(IndexType));
        }

        upload_buffer_region(vertices.data(), vsize, mVertexBuffer, voffset, context, cmd, stagingBuffers);
        upload_buffer_region(indices.data(), isize, mIndexBuffer, ioffset, context, cmd, stagingBuffers);

        auto handle = allocSlot();
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
        staging.upload(data, size);

        vk::BufferCopy region(0, dstOffset, size);
        cmd.copyBuffer(staging.getBuffer(), dst.getBuffer(), region);
    }

    MeshRegistry::MeshData MeshRegistry::generate_cube()
    {
        constexpr std::array<std::array<glm::vec3, 4>, 6> positions = {
            {
                {glm::vec3{-1, -1, 1}, glm::vec3{1, -1, 1}, glm::vec3{1, 1, 1}, glm::vec3{-1, 1, 1}},
                {glm::vec3{1, -1, -1}, glm::vec3{-1, -1, -1}, glm::vec3{-1, 1, -1}, glm::vec3{1, 1, -1}},
                {glm::vec3{-1, -1, -1}, glm::vec3{1, -1, -1}, glm::vec3{1, -1, 1}, glm::vec3{-1, -1, 1}},
                {glm::vec3{-1, 1, 1}, glm::vec3{1, 1, 1}, glm::vec3{1, 1, -1}, glm::vec3{-1, 1, -1}},
                {glm::vec3{-1, -1, -1}, glm::vec3{-1, -1, 1}, glm::vec3{-1, 1, 1}, glm::vec3{-1, 1, -1}},
                {glm::vec3{1, -1, 1}, glm::vec3{1, -1, -1}, glm::vec3{1, 1, -1}, glm::vec3{1, 1, 1}},
            }
        };
        constexpr std::array normals = {
            glm::vec3{0, 0, 1},
            glm::vec3{0, 0, -1},
            glm::vec3{0, -1, 0},
            glm::vec3{0, 1, 0},
            glm::vec3{-1, 0, 0},
            glm::vec3{1, 0, 0}
        };
        constexpr std::array uvs = {
            glm::vec2(0, 1),
            glm::vec2(1, 1),
            glm::vec2(1, 0),
            glm::vec2(0, 0)
        };

        MeshData data;

        for (int f = 0; f < 6; f++)
        {
            uint32_t base = static_cast<uint32_t>(data.vertices.size());
            for (int i = 0; i < 4; i++)
                data.vertices.emplace_back(
                    positions[f][i] * 0.5f,
                    normals[f],
                    uvs[i],
                    glm::vec4(1.f, 0.f, 0.f, 1.f)
                );

            data.indices.insert(data.indices.end(), {
                                    base + 0, base + 1, base + 2,
                                    base + 0, base + 2, base + 3
                                });
        }
        return data;
    }

    MeshRegistry::MeshData MeshRegistry::generate_sphere(uint32_t sectors, uint32_t stacks)
    {
        MeshData data;
        data.vertices.reserve((stacks + 1) * (sectors + 1));
        data.indices.reserve(stacks * sectors * 6);

        constexpr float radius = 1.f;

        const float sectorStep = 2.f * std::numbers::pi_v<float> / static_cast<float>(sectors);
        const float stackStep = std::numbers::pi_v<float> / static_cast<float>(stacks);
        constexpr float lengthInv = 1.f / radius;

        for (uint32_t i = 0; i <= stacks; ++i)
        {
            float stackAngle = std::numbers::pi_v<float> / 2.f - static_cast<float>(i) * stackStep;
            float xy = radius * std::cos(stackAngle);
            float z = radius * std::sin(stackAngle);

            for (uint32_t j = 0; j <= sectors; ++j)
            {
                float sectorAngle = static_cast<float>(j) * sectorStep;

                float x = xy * std::cos(sectorAngle);
                float y = xy * std::sin(sectorAngle);

                glm::vec3 pos(x, y, z);
                glm::vec3 normal = pos * lengthInv;
                glm::vec2 uv(
                    static_cast<float>(j) / static_cast<float>(sectors),
                    static_cast<float>(i) / static_cast<float>(stacks)
                );
                glm::vec4 tangent{-std::sin(sectorAngle), std::cos(sectorAngle), 0.f, 1.f};

                data.vertices.emplace_back(pos, normal, uv, tangent);
            }
        }

        for (uint32_t i = 0; i < stacks; ++i)
        {
            uint32_t k1 = i * (sectors + 1);
            uint32_t k2 = k1 + sectors + 1;

            for (uint32_t j = 0; j < sectors; ++j, ++k1, ++k2)
            {
                if (i != 0)
                {
                    data.indices.push_back(k1);
                    data.indices.push_back(k2);
                    data.indices.push_back(k1 + 1);
                }

                if (i != (stacks - 1))
                {
                    data.indices.push_back(k1 + 1);
                    data.indices.push_back(k2);
                    data.indices.push_back(k2 + 1);
                }
            }
        }

        return data;
    }

    MeshHandle MeshRegistry::upload(const Context &context,
                                    vk::CommandBuffer cmd, const MeshData &data, std::vector<Buffer> &stagingBuffer)
    {
        return uploadInternal(data.vertices, data.indices, context, cmd, stagingBuffer, false);
    }
}
