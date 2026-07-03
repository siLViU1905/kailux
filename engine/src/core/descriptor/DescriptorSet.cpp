#include "DescriptorSet.h"

#include "DescriptorPool.h"
#include "../Log.h"
#include "core/Pipeline.h"

namespace kailux
{
    DescriptorSet::DescriptorSet() : mSet({})
    {
    }

    DescriptorSet::DescriptorSet(DescriptorSet &&other) noexcept : mSet(std::move(other.mSet))
    {
    }

    DescriptorSet &DescriptorSet::operator=(DescriptorSet &&other) noexcept
    {
        if (this != &other)
        {
            mSet = std::move(other.mSet);
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
        return *mSet;
    }

    void DescriptorSet::bind(const Pipeline &pipeline, vk::CommandBuffer cmd, vk::PipelineBindPoint bindPoint) const
    {
        cmd.bindDescriptorSets(bindPoint, pipeline.getLayout(), 0, *mSet, {});
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
                            *mSet,
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
                            *mSet,
                            update.binding,
                            update.arrayIndex,
                            imageInfo.count,
                            imageInfo.type,
                            &imageInfos.back(),
                            nullptr
                        );
                    }
                },
                update.info
            );
        }
        context.mDevice.updateDescriptorSets(descriptorWrites, {});
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
        mSet = std::move(context.mDevice.allocateDescriptorSets(allocInfo).front());

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
                            *mSet,
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
                            *mSet,
                            i,
                            0,
                            imageInfo.count,
                            imageInfo.type,
                            &imageInfos[startIndex],
                            nullptr
                        );
                    }
                }, infos[i]);
        }

        context.mDevice.updateDescriptorSets(descriptorWrites, {});
    }
}
