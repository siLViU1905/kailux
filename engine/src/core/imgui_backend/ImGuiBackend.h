#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <imgui.h>
#include "core/Context.h"

namespace kailux
{
    class ImGuiBackend
    {
    public:
        ImGuiBackend();
        ImGuiBackend(const ImGuiBackend&) = delete;
        ImGuiBackend& operator=(const ImGuiBackend&) = delete;
        ImGuiBackend(ImGuiBackend&& other) noexcept;
        ImGuiBackend& operator=(ImGuiBackend&& other) noexcept;
        ~ImGuiBackend();

        static ImGuiBackend create(Window& window, const Context& context, const SwapChain& swapchain);

        void beginFrame();
        void endFrame();

        void shutdown();
    private:
        void createImGuiContext();
        void createDescriptorPool(const Context& context);
        void createImGuiVulkanContext(Window& window, const Context& context, const SwapChain& swapchain);

        ImGuiContext*            p_Context;
        ImGuiIO*                 p_IO;
        vk::raii::DescriptorPool m_DescriptorPool;
    };
}
