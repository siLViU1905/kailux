#pragma once
#include "core/Pipeline.h"
#include "core/descriptor/DescriptorPool.h"

namespace kailux
{
    class GraphicsPass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(GraphicsPass)
        virtual ~GraphicsPass() = default;

        void Bind(vk::CommandBuffer cmd) const;

        const DescriptorLayout& GetDescriptorLayout() const;
        const DescriptorPool&   GetDescriptorPool() const;
        const Pipeline&         GetPipeline() const;

    protected:
        void CreateDescriptorLayout(const Context &context, std::span<const DescriptorLayoutBinding> bindings);
        void CreateDescriptorPool(const Context &context, uint32_t frameCount, std::span<const DescriptorPoolSize> sizes);
        void CreatePipeline(const Context &context, const Swapchain &swapchain, std::string_view vertShaderPath, std::string_view fragShaderPath, const PipelineInfo& info, std::span<const PushConstantRangeInfo> pushConstantRanges);

        static constexpr bool check_descriptor_layout_bindings_and_pool_sizes_match(std::span<const DescriptorLayoutBinding> bindings, std::span<const DescriptorPoolSize> sizes)
        {
            if (bindings.size() != sizes.size())
                return false;

            for (size_t i = 0; i < bindings.size(); i++)
                if (bindings[i].type != sizes[i].type ||
                    bindings[i].count != sizes[i].count)
                    return false;

            return true;
        }

        template<auto PcRanges, typename... Pcs>
        void PushImpl(vk::CommandBuffer cmd, const Pcs &... pcs) const
        {
            static_assert(sizeof...(Pcs) == PcRanges.size(),
                  "Number of push constants doesnt correspond with kPushConstantRanges");

            uint32_t currentOffset = 0;
            size_t index = 0;

            ([&]() {
                assert(sizeof(Pcs) == PcRanges[index].size);

                cmd.pushConstants(
                    mPipeline.GetLayout(),
                    PcRanges[index].shaderStage,
                    currentOffset,
                    static_cast<uint32_t>(sizeof(Pcs)),
                    &pcs
                );

                currentOffset += static_cast<uint32_t>(sizeof(Pcs));
                ++index;
            }(), ...);
        }

        DescriptorLayout    mDescriptorLayout;
        DescriptorPool      mDescriptorPool;
        Pipeline            mPipeline;
    };
}
