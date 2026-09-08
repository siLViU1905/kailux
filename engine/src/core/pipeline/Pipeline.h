#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "../Context.h"
#include "../Core.h"
#include "../Swapchain.h"
#include "../descriptor/DescriptorLayout.h"

namespace kailux
{
    struct PushConstantRangeInfo
    {
        vk::ShaderStageFlagBits shaderStage;
        uint32_t                size{};
    };

    template<typename Derived>
    class Pipeline;

    template <class T>
    concept PipelineType = std::derived_from<T, Pipeline<T>>;

    template<typename Derived>
    class Pipeline
    {
    public:
        Pipeline() = default;

        Pipeline(const Pipeline &) = delete;

        Pipeline(Pipeline &&other) noexcept : mLayout(std::move(other.mLayout)),
                                              mPipeline(std::move(other.mPipeline))
        {
        }

        Pipeline &operator=(const Pipeline &) = delete;

        Pipeline &operator=(Pipeline &&other) noexcept
        {
            if (this != &other)
            {
                mLayout = std::move(other.mLayout);
                mPipeline = std::move(other.mPipeline);
            }
            return *this;
        }

        void Bind(vk::CommandBuffer cmd) const
        {
            cmd.bindPipeline(Derived::kBindPoint, *mPipeline);
        }

        vk::PipelineLayout GetLayout() const
        {
            return *mLayout;
        }

    protected:
        Pipeline(vk::raii::PipelineLayout &&layout, vk::raii::Pipeline &&pipeline) : mLayout(std::move(layout)),
                                                                                     mPipeline(std::move(pipeline))
        {
        }

        struct ShaderModuleInstance
        {
            vk::ShaderStageFlagBits stage;
            vk::raii::ShaderModule  module;
        };

        struct ShaderModules
        {
            std::vector<ShaderModuleInstance> modules;

            std::vector<vk::PipelineShaderStageCreateInfo> MakeVkStages() const
            {
                std::vector<vk::PipelineShaderStageCreateInfo> stages;
                stages.reserve(modules.size());

                for (const auto &instance: modules)
                {
                    stages.emplace_back(
                        vk::PipelineShaderStageCreateFlags{},
                        instance.stage,
                        *instance.module,
                        "main"
                    );
                }
                return stages;
            }
        };

        static vk::raii::PipelineLayout create_layout(const Context& context, const DescriptorLayout& descriptorSetLayout, std::span<const PushConstantRangeInfo> pushConstantRanges)
        {
            const auto dsLayout = descriptorSetLayout.GetLayout();

            std::vector<vk::PushConstantRange> ranges;
            ranges.reserve(pushConstantRanges.size());
            uint32_t offset = 0;
            for (auto [shaderStage, size]: pushConstantRanges)
            {
                ranges.emplace_back(
                    shaderStage,
                    offset,
                    size
                );
                offset += size;
            }

            const vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
                {},
                1,
                &dsLayout,
                static_cast<uint32_t>(ranges.size()),
                ranges.data()
            };

            return {context.mDevice, pipelineLayoutInfo};
        }

        template<typename... Args>
        static Derived make(
            const Context &context,
            const DescriptorLayout &descriptorSetLayout,
            std::span<const PushConstantRangeInfo> pushConstantRanges,
            Args &&... args)
        {
            auto layout{create_layout(context, descriptorSetLayout, pushConstantRanges)};
            auto pipeline{Derived::create_pipeline_impl(context, layout, std::forward<Args>(args)...)};

            return {
                std::move(layout),
                std::move(pipeline)
            };
        }

        vk::raii::PipelineLayout mLayout{nullptr};
        vk::raii::Pipeline       mPipeline{nullptr};
    };
}
