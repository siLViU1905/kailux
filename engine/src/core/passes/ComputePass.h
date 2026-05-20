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
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ComputePass)
        virtual ~ComputePass() = default;

        void bind(vk::CommandBuffer cmd) const;

        virtual void execute(vk::CommandBuffer cmd, ComputeWorkgroup group) const = 0;

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

        DescriptorLayout    m_DescriptorLayout;
        DescriptorPool      m_DescriptorPool;
        Pipeline            m_Pipeline;
    };
}
