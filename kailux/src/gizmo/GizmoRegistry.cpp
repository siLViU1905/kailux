#include "GizmoRegistry.h"

#include "buffer/BufferAllocator.h"

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
        registry.mVertexBuffer = buffer_allocator::alloc_vertex(context, kBuiltinZoneSize);
        registry.mIndexBuffer = buffer_allocator::alloc_index(context, kBuiltinZoneSize / 2);

        registry.mBuiltinVertexZone = {0, kBuiltinZoneSize, 0};
        registry.mBuiltinIndexZone = {0, kBuiltinZoneSize / 2, 0};

        auto uploadShape = [&](auto genFn, GizmoHandle &out)
        {
            auto data = genFn();
            out = registry.Upload(context, cmd, data, stagingBuffers);
        };

        uploadShape([]() { return gizmo_geometry::generate_point_light_gizmo(); }, registry.mBuiltins.pointLight);
        uploadShape([]() { return gizmo_geometry::generate_camera_gizmo(); }, registry.mBuiltins.camera);
        uploadShape([]() { return gizmo_geometry::generate_audio_source_gizmo(); }, registry.mBuiltins.audioSource);

        return registry;
    }

    GizmoRegistry::GizmoView GizmoRegistry::View(GizmoHandle handle) const
    {
        assert(handle.Valid());
        const auto &alloc = mAllocs[handle.index];
        return {
            static_cast<uint32_t>(alloc.indexOffset / sizeof(gizmo_geometry::IndexType)),
            alloc.indexCount,
            static_cast<int32_t>(alloc.vertexOffset / sizeof(GizmoVertex))
        };
    }

    void GizmoRegistry::Bind(vk::CommandBuffer cmd) const
    {
        cmd.bindVertexBuffers(0, mVertexBuffer.GetBuffer(), {0});
        cmd.bindIndexBuffer(mIndexBuffer.GetBuffer(), 0, vk::IndexType::eUint32);
    }

    GizmoRegistry::BuiltinGizmos GizmoRegistry::GetBuiltins() const
    {
        return mBuiltins;
    }

    GizmoHandle GizmoRegistry::AllocSlot()
    {
        mAllocs.emplace_back();
        return {static_cast<uint32_t>(mAllocs.size() - 1)};
    }

    GizmoHandle GizmoRegistry::Upload(const Context &context, vk::CommandBuffer cmd,
                                                     const gizmo_geometry::GizmoData &data, std::vector<Buffer> &stagingBuffers)
    {
        const auto& vertices = data.vertices;
        const auto& indices = data.indices;
        vk::DeviceSize vsize = vertices.size() * sizeof(GizmoVertex);
        vk::DeviceSize isize = indices.size() * sizeof(gizmo_geometry::IndexType);

        auto vOffset = mBuiltinVertexZone.Alloc(vsize, sizeof(GizmoVertex));
        auto iOffset = mBuiltinIndexZone.Alloc(isize, sizeof(gizmo_geometry::IndexType));

        upload_buffer_region(vertices.data(), vsize, mVertexBuffer, vOffset, context, cmd, stagingBuffers);
        upload_buffer_region(indices.data(), isize, mIndexBuffer, iOffset, context, cmd, stagingBuffers);

        auto handle = AllocSlot();
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
        auto &staging = stagingBuffers.emplace_back(buffer_allocator::alloc_staging(context, size));
        staging.Upload(data, size);

        vk::BufferCopy region(0, dstOffset, size);
        cmd.copyBuffer(staging.GetBuffer(), dst.GetBuffer(), region);
    }
}