#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <imgui.h>
#include "core/Context.h"
#include "../Swapchain.h"

namespace kailux
{
    class ImGuiBackend
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ImGuiBackend)
        ~ImGuiBackend();

        static ImGuiBackend create(Window& window, const Context& context, const Swapchain& swapchain, vk::SampleCountFlagBits sampleCount);

        void beginFrame();
        void endFrame();
        void recordDrawData(vk::CommandBuffer cmd) const;

        void shutdown();
    private:
        void createImGuiContext();
        void createDescriptorPool(const Context& context);
        void createImGuiVulkanContext(Window& window, const Context& context, const Swapchain& swapchain, vk::SampleCountFlagBits sampleCount);
        // this style was taken from https://github.com/ocornut/imgui/issues/707#issuecomment-3341945111
        void applyStyle();

        ImGuiContext*            p_Context;
        ImGuiIO*                 p_IO;
        vk::raii::DescriptorPool m_DescriptorPool;
    };
}
