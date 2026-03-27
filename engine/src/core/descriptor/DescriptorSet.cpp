#include "DescriptorSet.h"

#include "DescriptorPool.h"
#include "../Logger.h"
#include "core/Pipeline.h"

namespace kailux
{
    DescriptorSet::DescriptorSet() : m_Set({})
    {
    }

    DescriptorSet::DescriptorSet(DescriptorSet &&other) noexcept : m_Set(std::move(other.m_Set))
    {
    }

    DescriptorSet &DescriptorSet::operator=(DescriptorSet &&other) noexcept
    {
        if (this != &other)
        {
            m_Set = std::move(other.m_Set);
        }
        return *this;
    }

    DescriptorSet DescriptorSet::create(const Context &context, const DescriptorLayout &layout,
                                        const DescriptorPool &pool,
                                        std::span<DescriptorSetInfo> infos)
    {
        KAILUX_LOG_PARENT_CLR_GREEN("[DescriptorSet]")
        DescriptorSet set;

        set.createSet(context, layout, pool, infos);
        KAILUX_LOG_CHILD_CLR_GREEN(std::format("Created descriptor set with {} bindings", infos.size()))

        return set;
    }

    vk::DescriptorSet DescriptorSet::getDescriptorSet() const
    {
        return *m_Set;
    }

    void DescriptorSet::bind(const Pipeline &pipeline, vk::CommandBuffer cmd) const
    {
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.getLayout(), 0, *m_Set, {});
    }

    void DescriptorSet::createSet(const Context &context, const DescriptorLayout &layout, const DescriptorPool &pool,
                                  std::span<DescriptorSetInfo> infos)
    {
        auto layoutHandle = layout.getLayout();
        vk::DescriptorSetAllocateInfo allocInfo(
            pool.getPool(),
            1,
            &layoutHandle
        );
        m_Set = std::move(context.m_Device.allocateDescriptorSets(allocInfo).front());

        std::vector<vk::WriteDescriptorSet> descriptorWrites;

        std::vector<vk::DescriptorBufferInfo> bufferInfos;
        std::vector<vk::DescriptorImageInfo> imageInfos;

        descriptorWrites.reserve(infos.size());
        bufferInfos.reserve(infos.size());
        imageInfos.reserve(infos.size());

        for (uint32_t i = 0; i < infos.size(); ++i)
        {
            std::visit([&](const auto &arg)
            {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, DescriptorSetBufferInfo>)
                {
                    bufferInfos.emplace_back(arg.buffer, 0, arg.size);
                    descriptorWrites.emplace_back(
                        *m_Set,
                        i,
                        0,
                        arg.count,
                        arg.type,
                        nullptr,
                        &bufferInfos.back()
                    );
                } else if constexpr (std::is_same_v<T, DescriptorSetImageInfo>)
                {
                    imageInfos.emplace_back(arg.sampler, arg.view, arg.layout);
                    descriptorWrites.emplace_back(
                        *m_Set,
                        i,
                        0,
                        arg.count,
                        vk::DescriptorType::eCombinedImageSampler,
                        &imageInfos.back(),
                        nullptr
                    );
                }
            }, infos[i]);
        }

        context.m_Device.updateDescriptorSets(descriptorWrites, {});
    }
}
