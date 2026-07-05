#pragma once
#include "GizmoGeometry.h"
#include "../utilities/LinearZone.h"
#include "core/Context.h"
#include "core/Core.h"
#include "core/buffer/Buffer.h"

namespace kailux
{
    using GizmoHandle = Handle;

    class GizmoRegistry
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(GizmoRegistry)

        static GizmoRegistry create(const Context &context,
                                    vk::CommandBuffer cmd,
                                    std::vector<Buffer> &stagingBuffers);

        struct BuiltinGizmos
        {
            GizmoHandle pointLight;
        };

        struct GizmoAlloc
        {
            vk::DeviceSize vertexOffset{};
            uint32_t vertexCount{};
            vk::DeviceSize indexOffset{};
            uint32_t indexCount{};
        };

        struct GizmoView
        {
            uint32_t firstIndex{};
            uint32_t indexCount{};
            int32_t vertexOffset{};
        };

        GizmoView view(GizmoHandle handle) const;

        void bind(vk::CommandBuffer cmd) const;

        BuiltinGizmos getBuiltins() const;

    private:
        static constexpr vk::DeviceSize kVertexAlignment  = sizeof(GizmoVertex);
        static constexpr vk::DeviceSize kBuiltinZoneSize  = (4 * 1024 * 1024 / kVertexAlignment) * kVertexAlignment;

        GizmoHandle allocSlot();

        GizmoHandle upload(const Context &context,
                           vk::CommandBuffer cmd,
                           const GizmoGeometry::GizmoData &data,
                           std::vector<Buffer> &stagingBuffers);

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

        std::vector<GizmoAlloc> mAllocs;

        BuiltinGizmos mBuiltins;
    };
}
