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

        // for single push constant
        template<typename Pc, typename... Args>
        void push(vk::CommandBuffer cmd, Args&&... args) const
        {
            Pc pc{std::forward<Args>(args)...};

            cmd.pushConstants(
                    m_Pipeline.getLayout(),
                    vk::ShaderStageFlagBits::eCompute,
                    0,
                    sizeof(Pc),
                    &pc
                );
        }

        // for multiple push constants
        template<typename... Pc>
        void push(vk::CommandBuffer cmd, const Pc &... pcs) const
        {
            uint32_t currentOffset{};

            ([&]()
            {
                cmd.pushConstants(
                    m_Pipeline.getLayout(),
                    vk::ShaderStageFlagBits::eCompute,
                    currentOffset,
                    sizeof(Pc),
                    &pcs
                );
                currentOffset += sizeof(Pc);
            }(), ...);
        }

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

        DescriptorLayout    m_DescriptorLayout;
        DescriptorPool      m_DescriptorPool;
        Pipeline            m_Pipeline;
    };
}
