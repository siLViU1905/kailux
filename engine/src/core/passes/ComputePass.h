#pragma once
#include "core/Core.h"
#include "core/Pipeline.h"
#include "core/descriptor/DescriptorPool.h"

namespace kailux
{
    struct ComputeWorkgroup
    {
        uint32_t x{};
        uint32_t y{};
        uint32_t z{};
    };

    class ComputePass
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ComputePass);

        void bind(vk::CommandBuffer cmd) const;

        void execute(vk::CommandBuffer cmd, ComputeWorkgroup group) const;

        const DescriptorLayout& getDescriptorLayout() const;
        const DescriptorPool&   getDescriptorPool() const;
        const Pipeline&         getPipeline() const;

    protected:
        void createDescriptorLayout(const Context &context, std::span<const DescriptorLayoutBinding> bindings);
        void createDescriptorPool(const Context &context, uint32_t frameCount, std::span<const DescriptorPoolSize> sizes);
        void createPipeline(const Context &context, const ComputeShaderInfo &info, std
                            ::span<const PushConstantRangeInfo> pushConstantRanges);

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
        void pushImpl(vk::CommandBuffer cmd, const Pcs &... pcs) const
        {
            static_assert(sizeof...(Pcs) == PcRanges.size(),
                  "Number of push constants doesnt correspond with kPushConstantRanges");

            uint32_t currentOffset = 0;
            size_t index = 0;

            ([&]() {
                assert(sizeof(Pcs) == PcRanges[index].size);

                cmd.pushConstants(
                    mPipeline.getLayout(),
                    vk::ShaderStageFlagBits::eCompute,
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
