#include "MeshRegistry.h"
#include <numbers>

namespace kailux
{
    MeshRegistry::MeshRegistry()
    {
    }

    MeshRegistry::MeshRegistry(MeshRegistry &&other) noexcept : m_VertexBuffer(std::move(other.m_VertexBuffer)),
                                                                m_IndexBuffer(std::move(other.m_IndexBuffer)),
                                                                m_BuiltinVertexZone(other.m_BuiltinVertexZone),
                                                                m_BuiltinIndexZone(other.m_BuiltinIndexZone),
                                                                m_AssetVertexZone(std::move(other.m_AssetVertexZone)),
                                                                m_AssetIndexZone(std::move(other.m_AssetIndexZone)),
                                                                m_Allocs(std::move(other.m_Allocs)),
                                                                m_FreeSlots(std::move(other.m_FreeSlots)),
                                                                m_Builtins(other.m_Builtins)
    {
    }

    MeshRegistry &MeshRegistry::operator=(MeshRegistry &&other) noexcept
    {
        if (this != &other)
        {
            m_VertexBuffer = std::move(other.m_VertexBuffer);
            m_IndexBuffer = std::move(other.m_IndexBuffer);
            m_BuiltinVertexZone = other.m_BuiltinVertexZone;
            m_BuiltinIndexZone = other.m_BuiltinIndexZone;
            m_AssetVertexZone = std::move(other.m_AssetVertexZone);
            m_AssetIndexZone = std::move(other.m_AssetIndexZone);
            m_Allocs = std::move(other.m_Allocs);
            m_FreeSlots = std::move(other.m_FreeSlots);
            m_Builtins = other.m_Builtins;
        }
        return *this;
    }

    MeshRegistry MeshRegistry::create(const Context &context, vk::CommandBuffer cmd,
                                      std::vector<Buffer> &stagingBuffers)
    {
        MeshRegistry registry;
        registry.m_VertexBuffer = BufferAllocator::alloc_vertex(context, s_TotalSize);
        registry.m_IndexBuffer = BufferAllocator::alloc_index(context, s_TotalSize / 2);

        registry.m_BuiltinVertexZone = {0, s_BuiltinZoneSize, 0};
        registry.m_BuiltinIndexZone = {0, s_BuiltinZoneSize / 2, 0};
        registry.m_AssetVertexZone = {s_BuiltinZoneSize, s_AssetZoneSize};
        registry.m_AssetIndexZone = {s_BuiltinZoneSize / 2, s_AssetZoneSize / 2};

        registry.m_AssetVertexZone.freeBlocks.emplace_back(s_BuiltinZoneSize, s_AssetZoneSize);
        registry.m_AssetIndexZone.freeBlocks.emplace_back(s_BuiltinZoneSize / 2, s_AssetZoneSize / 2);

        auto uploadShape = [&](auto genFn, MeshHandle &out)
        {
            auto data = genFn();
            out = registry.uploadInternal(data.vertices, data.indices, context, cmd, stagingBuffers, true);
        };

        uploadShape([]() { return generate_cube(); }, registry.m_Builtins.cube);
        uploadShape([]() { return generate_sphere(); }, registry.m_Builtins.sphere);

        return registry;
    }

    MeshHandle MeshRegistry::upload(std::span<const Vertex> vertices, std::span<const IndexType> indices,
                                    const Context &context, vk::CommandBuffer cmd, std::vector<Buffer> &stagingBuffer)
    {
        return uploadInternal(vertices, indices, context, cmd, stagingBuffer, false);
    }

    void MeshRegistry::destroy(MeshHandle handle)
    {
        assert(handle.valid());
        auto &a = m_Allocs[handle.index];
        assert(!a.is_builtin && "Cannot destroy built-in shapes");

        m_AssetVertexZone.free(a.vertexOffset);
        m_AssetIndexZone.free(a.indexOffset);
        m_FreeSlots.push_back(handle.index);
        a = {};
    }

    MeshView MeshRegistry::view(MeshHandle handle) const
    {
        assert(handle.valid());
        const auto &alloc = m_Allocs[handle.index];
        return {
            static_cast<uint32_t>(alloc.indexOffset / sizeof(IndexType)),
            alloc.indexCount,
            static_cast<int32_t>(alloc.vertexOffset / sizeof(Vertex))
        };
    }

    std::vector<MeshView> MeshRegistry::viewAll() const
    {
        std::vector<MeshView> views;
        views.reserve(m_Allocs.size());
        for (const auto &alloc: m_Allocs)
            views.emplace_back(
                static_cast<uint32_t>(alloc.indexOffset / sizeof(IndexType)),
                alloc.indexCount,
                static_cast<int32_t>(alloc.vertexOffset / sizeof(Vertex))
            );
        return views;
    }

    void MeshRegistry::bind(vk::CommandBuffer cmd) const
    {
        cmd.bindVertexBuffers(0, m_VertexBuffer.getBuffer(), {0});
        cmd.bindIndexBuffer(m_IndexBuffer.getBuffer(), 0, vk::IndexType::eUint32);
    }

    uint32_t MeshRegistry::getMeshCount() const
    {
        return static_cast<uint32_t>(m_Allocs.size());
    }

    BuiltinMeshes MeshRegistry::getBuiltins() const
    {
        return m_Builtins;
    }

    vk::DeviceSize MeshRegistry::LinearZone::alloc(vk::DeviceSize size, vk::DeviceSize alignment)
    {
        vk::DeviceSize aligned = (cursor + alignment - 1) & ~(alignment - 1);
        if (aligned + size > capacity)
            throw std::runtime_error("Built in mesh zone out of memory");
        cursor = aligned + size;
        return base + aligned;
    }

    vk::DeviceSize MeshRegistry::FreeListZone::alloc(vk::DeviceSize size, vk::DeviceSize alignment)
    {
        for (auto it = freeBlocks.begin(); it != freeBlocks.end(); ++it)
        {
            vk::DeviceSize aligned = (it->offset + alignment - 1) & ~(alignment - 1);
            vk::DeviceSize padding = aligned - it->offset;

            if (it->size >= size + padding)
            {
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
        if (!m_FreeSlots.empty())
        {
            auto id = m_FreeSlots.back();
            m_FreeSlots.pop_back();
            return {id};
        }
        m_Allocs.emplace_back();

        return {static_cast<uint32_t>(m_Allocs.size() - 1)};
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
            voffset = m_BuiltinVertexZone.alloc(vsize, alignof(Vertex));
            ioffset = m_BuiltinIndexZone.alloc(isize, sizeof(IndexType));
        } else
        {
            voffset = m_AssetVertexZone.alloc(vsize, alignof(Vertex));
            ioffset = m_AssetIndexZone.alloc(isize, sizeof(IndexType));
        }

        upload_buffer_region(vertices.data(), vsize, m_VertexBuffer, voffset, context, cmd, stagingBuffers);
        upload_buffer_region(indices.data(), isize, m_IndexBuffer, ioffset, context, cmd, stagingBuffers);

        auto handle = allocSlot();
        m_Allocs[handle.index] = {
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

        vk::BufferMemoryBarrier2 barrier{
            vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eTransferWrite,
            vk::PipelineStageFlagBits2::eVertexInput,
            vk::AccessFlagBits2::eVertexAttributeRead | vk::AccessFlagBits2::eIndexRead,
            vk::QueueFamilyIgnored,
            vk::QueueFamilyIgnored,
            dst.getBuffer(),
            dstOffset,
            size
        };

        cmd.pipelineBarrier2(vk::DependencyInfo{}.setBufferMemoryBarriers(barrier));
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
                glm::vec4 tangent(
                    -radius * std::sin(stackAngle) * std::sin(sectorAngle),
                    radius * std::sin(stackAngle) * std::cos(sectorAngle),
                    0.f,
                    1.f
                );
                if (glm::length(glm::vec3(tangent)) > 0.0001f)
                    tangent = glm::vec4(glm::normalize(glm::vec3(tangent)), 1.0f);
                else
                    tangent = glm::vec4(1.f, 0.f, 0.f, 1.f);

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
}
