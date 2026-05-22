#include "ComputeCuller.h"

namespace kailux
{
    ComputeCuller::ComputeCuller() = default;

    ComputeCuller::ComputeCuller(ComputeCuller &&other) noexcept : ComputePass(std::move(other)),
                                                                   m_CullerPc(other.m_CullerPc)
    {
    }

    ComputeCuller &ComputeCuller::operator=(ComputeCuller &&other) noexcept
    {
        if (this != &other)
        {
            ComputePass::operator=(std::move(other));
            m_CullerPc = other.m_CullerPc;
        }
        return *this;
    }

    ComputeCuller ComputeCuller::create(const Context &context, uint32_t frameCount)
    {
        ComputeCuller picker;
        picker.createDescriptorLayout(context, s_DescriptorLayoutBindings);
        picker.createDescriptorPool(context, frameCount, s_DescriptorPoolSizes);
        picker.createPipeline(context, {s_ComputeShaderPath.data()}, s_PushConstantRanges);
        return picker;
    }

    void ComputeCuller::execute(vk::CommandBuffer cmd, ComputeWorkgroup group) const
    {
        cmd.pushConstants(
            m_Pipeline.getLayout(),
            vk::ShaderStageFlagBits::eCompute,
            0,
            sizeof(CullerPushConstant),
            &m_CullerPc
        );
        cmd.dispatch(group.x, group.y, group.z);
    }

    void ComputeCuller::setFrustum(const std::array<glm::vec4, 6> &planes, uint32_t totalObjects)
    {
        m_CullerPc.frustumPlanes = planes;
        m_CullerPc.totalObjects = totalObjects;
    }
}
