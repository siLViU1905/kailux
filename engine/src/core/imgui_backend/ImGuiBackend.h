#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <imgui.h>
#include "core/Context.h"
#include "../Swapchain.h"
#include "core/texture/Texture.h"

namespace kailux
{
    class ImGuiBackend
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ImGuiBackend)
        ~ImGuiBackend();

        static ImGuiBackend create(Window& window, const Context& context, const Swapchain& swapchain, vk::SampleCountFlagBits sampleCount);

        void updatePlatform();
        void beginFrame();
        void endFrame();
        void recordDrawData(vk::CommandBuffer cmd) const;

        void shutdown();

        static ImTextureID get_texture_id_from_texture(const Texture &texture);
        static void        remove_texture(ImTextureID id);

    private:
        static constexpr std::string_view kFontPath = "assets/fonts/JetBrainsMonoNL-Bold.ttf";

        void createImGuiContext();
        void createDescriptorPool(const Context& context);
        void createImGuiVulkanContext(Window& window, const Context& context, const Swapchain& swapchain, vk::SampleCountFlagBits sampleCount);
        void applyStyle();

        ImGuiContext*            p_Context;
        ImGuiIO*                 p_IO;
        VkFormat                 mColorAttachmentFormat{};
        vk::raii::DescriptorPool mDescriptorPool;
    };
}
