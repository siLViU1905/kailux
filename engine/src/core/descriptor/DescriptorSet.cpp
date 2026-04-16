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
                                        std::span<const DescriptorSetInfo> infos)
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

    void DescriptorSet::updateInfo(const Context &context, std::span<const DescriptorSetUpdateInfo> updateInfos) const
    {
        std::vector<vk::WriteDescriptorSet> descriptorWrites;
        std::vector<vk::DescriptorBufferInfo> bufferInfos;
        std::vector<vk::DescriptorImageInfo> imageInfos;

        descriptorWrites.reserve(updateInfos.size());

        bufferInfos.reserve(updateInfos.size());
        imageInfos.reserve(updateInfos.size());

        for (const auto &update: updateInfos)
        {
            std::visit(
                VisitOverloads{
                    [&](const DescriptorSetBufferInfo &bufferInfo)
                    {
                        bufferInfos.emplace_back(
                            bufferInfo.buffer,
                            0,
                            bufferInfo.size
                        );

                        descriptorWrites.emplace_back(
                            *m_Set,
                            update.binding,
                            update.arrayIndex,
                            bufferInfo.count,
                            bufferInfo.type,
                            nullptr,
                            &bufferInfos.back()
                        );
                    },
                    [&](const DescriptorSetImageInfo &imageInfo)
                    {
                        imageInfos.emplace_back(
                            imageInfo.sampler,
                            imageInfo.view,
                            imageInfo.layout
                        );

                        descriptorWrites.emplace_back(
                            *m_Set,
                            update.binding,
                            update.arrayIndex,
                            imageInfo.count,
                            vk::DescriptorType::eCombinedImageSampler,
                            &imageInfos.back(),
                            nullptr
                        );
                    }
                },
                update.info
            );
        }
        context.m_Device.updateDescriptorSets(descriptorWrites, {});
    }

    void DescriptorSet::createSet(const Context &context, const DescriptorLayout &layout, const DescriptorPool &pool,
                                  std::span<const DescriptorSetInfo> infos)
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

        uint32_t totalBufferCount = 0;
        uint32_t totalImageCount = 0;
        for (const auto &info: infos)
            std::visit(
                VisitOverloads
                {
                    [&](const DescriptorSetBufferInfo &bufferInfo)
                    {
                        totalBufferCount += bufferInfo.count;
                    },
                    [&](const DescriptorSetImageInfo &imageInfo)
                    {
                        totalImageCount += imageInfo.count;
                    }
                }, info);

        descriptorWrites.reserve(infos.size());
        bufferInfos.reserve(totalBufferCount);
        imageInfos.reserve(totalImageCount);

        for (uint32_t i = 0; i < infos.size(); ++i)
        {
            std::visit(
                VisitOverloads
                {
                    [&](const DescriptorSetBufferInfo &bufferInfo)
                    {
                        auto startIndex = static_cast<uint32_t>(bufferInfos.size());
                        for (uint32_t c = 0; c < bufferInfo.count; ++c)
                            bufferInfos.emplace_back(bufferInfo.buffer, 0, bufferInfo.size);

                        descriptorWrites.emplace_back(
                            *m_Set,
                            i,
                            0,
                            bufferInfo.count,
                            bufferInfo.type,
                            nullptr,
                            &bufferInfos[startIndex]
                        );
                    },
                    [&](const DescriptorSetImageInfo &imageInfo)
                    {
                        auto startIndex = static_cast<uint32_t>(imageInfos.size());
                        for (uint32_t c = 0; c < imageInfo.count; ++c)
                            imageInfos.emplace_back(imageInfo.sampler, imageInfo.view, imageInfo.layout);

                        descriptorWrites.emplace_back(
                            *m_Set,
                            i,
                            0,
                            imageInfo.count,
                            vk::DescriptorType::eCombinedImageSampler,
                            &imageInfos[startIndex],
                            nullptr
                        );
                    }
                }, infos[i]);
        }

        context.m_Device.updateDescriptorSets(descriptorWrites, {});
    }
}
