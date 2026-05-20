#pragma once
#include "../Pipeline.h"
#include "../descriptor/DescriptorLayout.h"
#include "../descriptor/DescriptorPool.h"

namespace kailux
{
    class ComputePicker
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ComputePicker)

        static ComputePicker create(const Context &context, uint32_t frameCount);

        void bind(vk::CommandBuffer cmd) const;

        void execute(vk::CommandBuffer cmd) const;

        const DescriptorLayout& getDescriptorLayout() const;
        const DescriptorPool&   getDescriptorPool() const;
        const Pipeline&         getPipeline() const;

        void setCords(uint32_t x, uint32_t y);

    private:
        static constexpr std::string_view s_PickerComputeShaderPath = "shaders/entity_picker_compute_shader.spv";

        static constexpr std::array s_DescriptorLayoutBindings = {
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageImage,
                1, // in id's image
                vk::ShaderStageFlagBits::eCompute
            ),
            DescriptorLayoutBinding(
                vk::DescriptorType::eStorageBuffer,
                1, // out id
                vk::ShaderStageFlagBits::eCompute
            )
        };
        static constexpr std::array s_DescriptorLayoutSizes = {
            DescriptorPoolSize(
                vk::DescriptorType::eStorageImage,
                1 // id's image
            ),
            DescriptorPoolSize(
                vk::DescriptorType::eStorageBuffer,
                1 // out id
            )
        };

        static constexpr std::array s_PushConstantRanges = {
            PushConstantRangeInfo(
                vk::ShaderStageFlagBits::eCompute,
                sizeof(uint32_t) * 2
            )
        };

        void createDescriptorLayout(const Context &context);
        void createDescriptorPool(const Context &context, uint32_t frameCount);
        void createPipeline(const Context &context, std::string_view shaderPath);

        DescriptorLayout m_DescriptorLayout;
        DescriptorPool   m_DescriptorPool;
        Pipeline         m_Pipeline;

        uint32_t         m_CordX;
        uint32_t         m_CordY;
    };
}
