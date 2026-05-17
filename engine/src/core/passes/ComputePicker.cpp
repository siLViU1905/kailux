#include "ComputePicker.h"

namespace kailux
{
    ComputePicker::ComputePicker() : m_CordX(0), m_CordY(0)
    {
    }

    ComputePicker::ComputePicker(ComputePicker &&other) noexcept : m_DescriptorLayout(std::move(other.m_DescriptorLayout)),
                                                                   m_DescriptorPool(std::move(other.m_DescriptorPool)),
                                                                   m_Pipeline(std::move(other.m_Pipeline)),
                                                                   m_CordX(other.m_CordX),
                                                                   m_CordY(other.m_CordY)
    {
    }

    ComputePicker &ComputePicker::operator=(ComputePicker &&other) noexcept
    {
        if (this != &other)
        {
            m_DescriptorLayout = std::move(other.m_DescriptorLayout);
            m_DescriptorPool = std::move(other.m_DescriptorPool);
            m_Pipeline = std::move(other.m_Pipeline);
            m_CordX = other.m_CordX;
            m_CordY = other.m_CordY;
        }
        return *this;
    }

    ComputePicker ComputePicker::create(const Context &context, uint32_t frameCount, std::string_view shaderPath)
    {
        ComputePicker picker;
        picker.createDescriptorLayout(context);
        picker.createDescriptorPool(context, frameCount);
        picker.createPipeline(context, shaderPath);
        return picker;
    }

    void ComputePicker::bind(vk::CommandBuffer cmd) const
    {
        m_Pipeline.bindCompute(cmd);
    }

    void ComputePicker::execute(vk::CommandBuffer cmd) const
    {
        std::array cords = {m_CordX, m_CordY};
        cmd.pushConstants(
            m_Pipeline.getLayout(),
            vk::ShaderStageFlagBits::eCompute,
            0,
            sizeof(uint32_t) * 2,
            cords.data()
        );
        cmd.dispatch(1, 1, 1);
    }

    const DescriptorLayout &ComputePicker::getDescriptorLayout() const
    {
        return m_DescriptorLayout;
    }

    const DescriptorPool &ComputePicker::getDescriptorPool() const
    {
        return m_DescriptorPool;
    }

    const Pipeline &ComputePicker::getPipeline() const
    {
        return m_Pipeline;
    }

    void ComputePicker::setCords(uint32_t x, uint32_t y)
    {
        m_CordX = x;
        m_CordY = y;
    }

    void ComputePicker::createDescriptorLayout(const Context &context)
    {
        m_DescriptorLayout = DescriptorLayout::create(context, s_DescriptorLayoutBindings);
    }

    void ComputePicker::createDescriptorPool(const Context &context, uint32_t frameCount)
    {
        m_DescriptorPool = DescriptorPool::create(context, frameCount, s_DescriptorLayoutSizes);
    }

    void ComputePicker::createPipeline(const Context &context, std::string_view shaderPath)
    {
        m_Pipeline = Pipeline::createCompute(context, m_DescriptorLayout, {shaderPath.data()}, s_PushConstantRanges);
    }
}
