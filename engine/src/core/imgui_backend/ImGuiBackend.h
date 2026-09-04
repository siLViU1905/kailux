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

        void UpdatePlatform();
        void BeginFrame();
        void EndFrame();
        void RecordDrawData(vk::CommandBuffer cmd) const;

        void Shutdown();

        static ImTextureID get_texture_id_from_texture(const Texture &texture);
        static void        remove_texture(ImTextureID id);

    private:
        static constexpr std::string_view kFontPath = "assets/fonts/JetBrainsMonoNL-Bold.ttf";

        void CreateImGuiContext();
        void CreateDescriptorPool(const Context& context);
        void CreateImGuiVulkanContext(Window& window, const Context& context, const Swapchain& swapchain, vk::SampleCountFlagBits sampleCount);
        void ApplyStyle();

        ImGuiContext*            p_Context;
        ImGuiIO*                 p_IO;
        VkFormat                 mColorAttachmentFormat{};
        vk::raii::DescriptorPool mDescriptorPool;
    };
}
