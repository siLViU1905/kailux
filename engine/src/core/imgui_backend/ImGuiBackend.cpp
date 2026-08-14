#include "ImGuiBackend.h"
#include "../Log.h"
#include "bindings/imgui_impl_glfw.h"
#include "bindings/imgui_impl_vulkan.h"
#include "../Swapchain.h"
#include <ImGuizmo.h>

namespace kailux
{
    ImGuiBackend::ImGuiBackend() : p_Context(nullptr), p_IO(nullptr), mDescriptorPool({})
    {
    }

    ImGuiBackend::ImGuiBackend(ImGuiBackend &&other) noexcept : p_Context(other.p_Context),
                                                                p_IO(other.p_IO),
                                                                mDescriptorPool(std::move(other.mDescriptorPool))
    {
        other.p_Context = nullptr;
        other.p_IO = nullptr;
    }

    ImGuiBackend &ImGuiBackend::operator=(ImGuiBackend &&other) noexcept
    {
        if (this != &other)
        {
            p_Context = other.p_Context;
            p_IO = other.p_IO;
            mDescriptorPool = std::move(other.mDescriptorPool);

            other.p_Context = nullptr;
            other.p_IO = nullptr;
        }
        return *this;
    }

    ImGuiBackend::~ImGuiBackend()
    {
        if (p_Context)
        {
            shutdown();

            ImGui::DestroyContext(p_Context);

            p_Context = nullptr;

            p_IO = nullptr;
        }
    }

    ImGuiBackend ImGuiBackend::create(Window &window, const Context &context, const Swapchain &swapchain,
                                      vk::SampleCountFlagBits sampleCount)
    {
        log::console.debug("imgui backend: creating");
        ImGuiBackend imguiBackend;

        imguiBackend.createImGuiContext();
        log::console.debug("imgui backend: imgui context created");

        imguiBackend.createDescriptorPool(context);
        log::console.debug("imgui backend: descriptor pool created");

        imguiBackend.createImGuiVulkanContext(window, context, swapchain, sampleCount);
        log::console.debug("imgui backend: imgui vulkan context created");

        imguiBackend.applyStyle();
        log::console.debug("imgui backend: style applied");

        return imguiBackend;
    }

    void ImGuiBackend::beginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::BeginFrame();
    }

    void ImGuiBackend::endFrame()
    {
        ImGui::Render();
    }

    void ImGuiBackend::recordDrawData(vk::CommandBuffer cmd) const
    {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    }

    void ImGuiBackend::shutdown()
    {
        ImGui_ImplVulkan_Shutdown();

        ImGui_ImplGlfw_Shutdown();
    }

    ImTextureID ImGuiBackend::get_texture_id_from_texture(const Texture &texture)
    {
        auto descriptorSet = ImGui_ImplVulkan_AddTexture(
            texture.getSampler(),
            texture.getImageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        return reinterpret_cast<ImTextureID>(descriptorSet);
    }

    void ImGuiBackend::remove_texture(ImTextureID id)
    {
        ImGui_ImplVulkan_RemoveTexture(reinterpret_cast<VkDescriptorSet>(id));
    }

    void ImGuiBackend::createDescriptorPool(const Context &context)
    {
        constexpr uint32_t descriptorCount = 1000;

        constexpr std::array poolSizes = {
            vk::DescriptorPoolSize(vk::DescriptorType::eSampler, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, descriptorCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, descriptorCount)
        };

        vk::DescriptorPoolCreateInfo poolInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            descriptorCount * poolSizes.size(),
            poolSizes.size(),
            poolSizes.data()
        );

        mDescriptorPool = vk::raii::DescriptorPool(context.mDevice, poolInfo);
    }

    void ImGuiBackend::createImGuiContext()
    {
        IMGUI_CHECKVERSION();

        p_Context = ImGui::CreateContext();

        p_IO = &ImGui::GetIO();
        p_IO->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        p_IO->IniFilename = "assets/ini/imgui.ini";
    }

    void ImGuiBackend::createImGuiVulkanContext(Window &window, const Context &context, const Swapchain &swapchain,
                                                vk::SampleCountFlagBits sampleCount)
    {
        ImGui_ImplGlfw_InitForVulkan(window.getGLFWWindow(), true);

        ImGui_ImplVulkan_InitInfo initInfo{};

        initInfo.Instance = *context.mInstance;
        initInfo.PhysicalDevice = *context.mPhysicalDevice;
        initInfo.Device = *context.mDevice;
        initInfo.QueueFamily = context.getGraphicsQueueFamilyIndex();
        initInfo.Queue = *context.mGraphicsQueue;
        initInfo.DescriptorPool = *mDescriptorPool;
        initInfo.MinImageCount = swapchain.getImageCount();
        initInfo.ImageCount = swapchain.getImageCount();
        initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Allocator = nullptr;
        initInfo.CheckVkResultFn = nullptr;

        initInfo.UseDynamicRendering = true;
        initInfo.PipelineInfoMain.RenderPass = VK_NULL_HANDLE;

        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;

        auto colorFormat = swapchain.getFormat();
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = reinterpret_cast<const VkFormat
            *>(&colorFormat);
        auto depthFormat = swapchain.getDepthFormat();
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;

        if (!ImGui_ImplVulkan_Init(&initInfo))
            throw std::runtime_error("Failed to initialize ImGui for Vulkan");
    }

    void ImGuiBackend::applyStyle()
    {
        p_IO->Fonts->AddFontFromFileTTF(kFontPath.data(), 20.f);

        auto &style = ImGui::GetStyle();

        constexpr auto to_rgba = [](uint32_t argb) constexpr
        {
            return ImVec4{
                ((argb >> 16) & 0xFF) / 255.f,
                ((argb >> 8) & 0xFF) / 255.f,
                (argb & 0xFF) / 255.f,
                ((argb >> 24) & 0xFF) / 255.f
            };
        };

        constexpr auto alpha = [](const ImVec4 &c, float a) constexpr
        {
            return ImVec4{c.x, c.y, c.z, a};
        };

        constexpr auto lerp = [](const ImVec4 &a, const ImVec4 &b, float t) constexpr
        {
            return ImVec4{
                std::lerp(a.x, b.x, t),
                std::lerp(a.y, b.y, t),
                std::lerp(a.z, b.z, t),
                std::lerp(a.w, b.w, t)
            };
        };

        constexpr ImVec4 kVoid = to_rgba(0xFF121212);
        constexpr ImVec4 kSunken = to_rgba(0xFF191919);
        constexpr ImVec4 kPanel = to_rgba(0xFF1F1F1F);
        constexpr ImVec4 kRaised = to_rgba(0xFF292929);
        constexpr ImVec4 kOverlay = to_rgba(0xFF313131);

        constexpr ImVec4 kBorder = to_rgba(0xFF3A3A3A);
        constexpr ImVec4 kBorderStrong = to_rgba(0xFF525252);

        constexpr ImVec4 kText = to_rgba(0xFFC3C3C3);
        constexpr ImVec4 kTextBright = to_rgba(0xFFF2F2F2);
        constexpr ImVec4 kTextDim = to_rgba(0xFF6E6E6E);

        constexpr ImVec4 kWhite = to_rgba(0xFFFFFFFF);

        constexpr ImVec4 kAccent = to_rgba(0xFFE8A24A);
        constexpr ImVec4 kAccentDim = to_rgba(0xFFB57B33);
        constexpr ImVec4 kSuccess = to_rgba(0xFF8FBF6B);
        constexpr ImVec4 kDanger = to_rgba(0xFFE05252);
        constexpr ImVec4 kInfo = to_rgba(0xFF5C9FD6);

        style.WindowPadding = ImVec2(12.f, 11.f);
        style.FramePadding = ImVec2(11.f, 7.f);
        style.ItemSpacing = ImVec2(9.f, 8.f);
        style.ItemInnerSpacing = ImVec2(8.f, 6.f);
        style.CellPadding = ImVec2(8.f, 5.f);
        style.IndentSpacing = 22.f;
        style.ScrollbarSize = 13.f;
        style.GrabMinSize = 12.f;

        style.WindowBorderSize = 0.f;
        style.ChildBorderSize = 1.f;
        style.PopupBorderSize = 1.f;
        style.FrameBorderSize = 1.f;
        style.SeparatorTextBorderSize = 1.f;

        style.WindowRounding = 8.f;
        style.ChildRounding = 7.f;
        style.PopupRounding = 8.f;
        style.FrameRounding = 5.f;
        style.ScrollbarRounding = 12.f;
        style.GrabRounding = 5.f;
        style.TabRounding = 6.f;

        style.WindowTitleAlign = ImVec2(0.f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_None;
        style.SeparatorTextAlign = ImVec2(0.f, 0.5f);
        style.SeparatorTextPadding = ImVec2(16.f, 6.f);
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.f, 0.5f);
        style.ColorButtonPosition = ImGuiDir_Left;
        style.DisabledAlpha = 0.38f;

        style.DockingSeparatorSize = 4.f;
        style.TabBarBorderSize = 1.f;
        style.TabBarOverlineSize = 2.f;
        style.TabCloseButtonMinWidthSelected = -1.f;
        style.TabCloseButtonMinWidthUnselected = 0.f;

        style.TreeLinesFlags = ImGuiTreeNodeFlags_DrawLinesToNodes;
        style.TreeLinesSize = 1.f;
        style.TreeLinesRounding = 8.f;

        style.HoverStationaryDelay = 0.12f;
        style.HoverDelayShort = 0.10f;
        style.HoverDelayNormal = 0.30f;
        style.HoverFlagsForTooltipMouse =
                ImGuiHoveredFlags_Stationary | ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled;

        style.AntiAliasedLines = true;
        style.AntiAliasedLinesUseTex = true;
        style.AntiAliasedFill = true;

        auto *colors = style.Colors;

        colors[ImGuiCol_Text] = kText;
        colors[ImGuiCol_TextDisabled] = kTextDim;
        colors[ImGuiCol_TextSelectedBg] = alpha(kAccent, 0.30f);
        colors[ImGuiCol_TextLink] = kAccent;

        colors[ImGuiCol_WindowBg] = kVoid;
        colors[ImGuiCol_ChildBg] = ImVec4{0.f, 0.f, 0.f, 0.f};
        colors[ImGuiCol_PopupBg] = kOverlay;
        colors[ImGuiCol_Border] = alpha(kBorder, 0.90f);
        colors[ImGuiCol_BorderShadow] = ImVec4{0.f, 0.f, 0.f, 0.f};

        colors[ImGuiCol_FrameBg] = kSunken;
        colors[ImGuiCol_FrameBgHovered] = alpha(kWhite, 0.05f);
        colors[ImGuiCol_FrameBgActive] = alpha(kWhite, 0.09f);

        colors[ImGuiCol_TitleBg] = kSunken;
        colors[ImGuiCol_TitleBgActive] = kRaised;
        colors[ImGuiCol_TitleBgCollapsed] = alpha(kSunken, 0.70f);
        colors[ImGuiCol_MenuBarBg] = kSunken;

        colors[ImGuiCol_ScrollbarBg] = ImVec4{0.f, 0.f, 0.f, 0.f};
        colors[ImGuiCol_ScrollbarGrab] = alpha(kWhite, 0.10f);
        colors[ImGuiCol_ScrollbarGrabHovered] = alpha(kWhite, 0.18f);
        colors[ImGuiCol_ScrollbarGrabActive] = kAccentDim;

        colors[ImGuiCol_CheckMark] = kAccent;
        colors[ImGuiCol_SliderGrab] = kAccentDim;
        colors[ImGuiCol_SliderGrabActive] = kAccent;

        colors[ImGuiCol_Button] = alpha(kWhite, 0.06f);
        colors[ImGuiCol_ButtonHovered] = alpha(kWhite, 0.13f);
        colors[ImGuiCol_ButtonActive] = alpha(kAccent, 0.55f);

        colors[ImGuiCol_Header] = alpha(kAccent, 0.20f);
        colors[ImGuiCol_HeaderHovered] = alpha(kWhite, 0.07f);
        colors[ImGuiCol_HeaderActive] = alpha(kAccent, 0.32f);

        colors[ImGuiCol_Separator] = alpha(kBorder, 0.80f);
        colors[ImGuiCol_SeparatorHovered] = alpha(kAccent, 0.65f);
        colors[ImGuiCol_SeparatorActive] = kAccent;

        colors[ImGuiCol_ResizeGrip] = alpha(kWhite, 0.08f);
        colors[ImGuiCol_ResizeGripHovered] = alpha(kAccent, 0.50f);
        colors[ImGuiCol_ResizeGripActive] = kAccent;

        colors[ImGuiCol_InputTextCursor] = kAccent;

        colors[ImGuiCol_Tab] = kSunken;
        colors[ImGuiCol_TabHovered] = alpha(kWhite, 0.10f);
        colors[ImGuiCol_TabSelected] = kRaised;
        colors[ImGuiCol_TabSelectedOverline] = kAccent;
        colors[ImGuiCol_TabDimmed] = lerp(kSunken, kPanel, 0.55f);
        colors[ImGuiCol_TabDimmedSelected] = lerp(kRaised, kPanel, 0.45f);
        colors[ImGuiCol_TabDimmedSelectedOverline] = alpha(kAccent, 0.28f);

        colors[ImGuiCol_DockingPreview] = alpha(kAccent, 0.38f);
        colors[ImGuiCol_DockingEmptyBg] = kVoid;

        colors[ImGuiCol_PlotLines] = kInfo;
        colors[ImGuiCol_PlotLinesHovered] = kTextBright;
        colors[ImGuiCol_PlotHistogram] = kAccent;
        colors[ImGuiCol_PlotHistogramHovered] = kDanger;

        colors[ImGuiCol_TableHeaderBg] = kSunken;
        colors[ImGuiCol_TableBorderStrong] = alpha(kBorderStrong, 0.55f);
        colors[ImGuiCol_TableBorderLight] = alpha(kBorder, 0.55f);
        colors[ImGuiCol_TableRowBg] = ImVec4{0.f, 0.f, 0.f, 0.f};
        colors[ImGuiCol_TableRowBgAlt] = alpha(kWhite, 0.025f);

        colors[ImGuiCol_TreeLines] = alpha(kBorder, 0.85f);
        colors[ImGuiCol_DragDropTarget] = kSuccess;

        colors[ImGuiCol_NavCursor] = kAccent;
        colors[ImGuiCol_NavWindowingHighlight] = kTextBright;
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4{0.f, 0.f, 0.f, 0.50f};
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4{0.f, 0.f, 0.f, 0.60f};
    }
}
