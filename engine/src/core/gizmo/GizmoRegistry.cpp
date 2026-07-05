#include "GizmoRegistry.h"

#include "core/buffer/BufferAllocator.h"

namespace kailux
{
    GizmoRegistry::GizmoRegistry() = default;

    GizmoRegistry::GizmoRegistry(GizmoRegistry &&other) noexcept : mVertexBuffer(std::move(other.mVertexBuffer)),
                                                                   mIndexBuffer(std::move(other.mIndexBuffer)),
                                                                   mBuiltinVertexZone(other.mBuiltinVertexZone),
                                                                   mBuiltinIndexZone(other.mBuiltinIndexZone),
                                                                   mAllocs(std::move(other.mAllocs)),
                                                                   mBuiltins(other.mBuiltins)
    {
    }

    GizmoRegistry &GizmoRegistry::operator=(GizmoRegistry &&other) noexcept
    {
        if (this != &other)
        {
            mVertexBuffer = std::move(other.mVertexBuffer);
            mIndexBuffer = std::move(other.mIndexBuffer);
            mBuiltinVertexZone = other.mBuiltinVertexZone;
            mBuiltinIndexZone = other.mBuiltinIndexZone;
            mAllocs = std::move(other.mAllocs);
            mBuiltins = other.mBuiltins;
        }
        return *this;
    }

    GizmoRegistry GizmoRegistry::create(const Context &context, vk::CommandBuffer cmd,
        std::vector<Buffer> &stagingBuffers)
    {
        GizmoRegistry registry;
        registry.mVertexBuffer = BufferAllocator::alloc_vertex(context, kBuiltinZoneSize);
        registry.mIndexBuffer = BufferAllocator::alloc_index(context, kBuiltinZoneSize / 2);

        registry.mBuiltinVertexZone = {0, kBuiltinZoneSize, 0};
        registry.mBuiltinIndexZone = {0, kBuiltinZoneSize / 2, 0};

        auto uploadShape = [&](auto genFn, GizmoHandle &out)
        {
            auto data = genFn();
            out = registry.upload(context, cmd, data, stagingBuffers);
        };

        uploadShape([]() { return GizmoGeometry::generate_point_light_gizmo(); }, registry.mBuiltins.pointLight);

        return registry;
    }

    GizmoRegistry::GizmoView GizmoRegistry::view(GizmoHandle handle) const
    {
        assert(handle.valid());
        const auto &alloc = mAllocs[handle.index];
        return {
            static_cast<uint32_t>(alloc.indexOffset / sizeof(GizmoGeometry::IndexType)),
            alloc.indexCount,
            static_cast<int32_t>(alloc.vertexOffset / sizeof(GizmoVertex))
        };
    }

    void GizmoRegistry::bind(vk::CommandBuffer cmd) const
    {
        cmd.bindVertexBuffers(0, mVertexBuffer.getBuffer(), {0});
        cmd.bindIndexBuffer(mIndexBuffer.getBuffer(), 0, vk::IndexType::eUint32);
    }

    GizmoRegistry::BuiltinGizmos GizmoRegistry::getBuiltins() const
    {
        return mBuiltins;
    }

    GizmoHandle GizmoRegistry::allocSlot()
    {
        mAllocs.emplace_back();
        return {static_cast<uint32_t>(mAllocs.size() - 1)};
    }

    GizmoHandle GizmoRegistry::upload(const Context &context, vk::CommandBuffer cmd,
                                                     const GizmoGeometry::GizmoData &data, std::vector<Buffer> &stagingBuffers)
    {
        const auto& vertices = data.vertices;
        const auto& indices = data.indices;
        vk::DeviceSize vsize = vertices.size() * sizeof(GizmoVertex);
        vk::DeviceSize isize = indices.size() * sizeof(GizmoGeometry::IndexType);

        auto vOffset = mBuiltinVertexZone.alloc(vsize, sizeof(GizmoVertex));
        auto iOffset = mBuiltinIndexZone.alloc(isize, sizeof(GizmoGeometry::IndexType));

        upload_buffer_region(vertices.data(), vsize, mVertexBuffer, vOffset, context, cmd, stagingBuffers);
        upload_buffer_region(indices.data(), isize, mIndexBuffer, iOffset, context, cmd, stagingBuffers);

        auto handle = allocSlot();
        mAllocs[handle.index] = {
            vOffset,
            static_cast<uint32_t>(vertices.size()),
            iOffset,
            static_cast<uint32_t>(indices.size())
        };
        return handle;
    }

    void GizmoRegistry::upload_buffer_region(const void *data, vk::DeviceSize size, Buffer &dst,
        vk::DeviceSize dstOffset, const Context &context, vk::CommandBuffer cmd, std::vector<Buffer> &stagingBuffers)
    {
        auto &staging = stagingBuffers.emplace_back(BufferAllocator::alloc_staging(context, size));
        staging.upload(data, size);

        vk::BufferCopy region(0, dstOffset, size);
        cmd.copyBuffer(staging.getBuffer(), dst.getBuffer(), region);
    }
}