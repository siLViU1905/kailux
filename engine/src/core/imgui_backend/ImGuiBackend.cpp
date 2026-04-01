#include "ImGuiBackend.h"
#include "../Logger.h"
#include "bindings/imgui_impl_glfw.h"
#include "bindings/imgui_impl_vulkan.h"
#include "../Swapchain.h"
#include <ImGuizmo.h>

namespace kailux
{
    ImGuiBackend::ImGuiBackend() : p_Context(nullptr), p_IO(nullptr), m_DescriptorPool({})
    {
    }

    ImGuiBackend::ImGuiBackend(ImGuiBackend &&other) noexcept : p_Context(other.p_Context),
                                                                p_IO(other.p_IO),
                                                                m_DescriptorPool(std::move(other.m_DescriptorPool))
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
            m_DescriptorPool = std::move(other.m_DescriptorPool);

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
        KAILUX_LOG_PARENT_CLR_MAGENTA("[IMGUI BACKEND]")
        ImGuiBackend imguiBackend;

        imguiBackend.createImGuiContext();
        KAILUX_LOG_CHILD_CLR_MAGENTA("ImGui context created")

        imguiBackend.createDescriptorPool(context);
        KAILUX_LOG_CHILD_CLR_MAGENTA("Descriptor pool created")

        imguiBackend.createImGuiVulkanContext(window, context, swapchain, sampleCount);
        KAILUX_LOG_CHILD_CLR_MAGENTA("ImGui Vulkan context created")

        imguiBackend.applyStyle();
        KAILUX_LOG_CHILD_CLR_MAGENTA("Style applied")

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

        m_DescriptorPool = vk::raii::DescriptorPool(context.m_Device, poolInfo);
    }

    void ImGuiBackend::createImGuiContext()
    {
        IMGUI_CHECKVERSION();

        p_Context = ImGui::CreateContext();

        p_IO = &ImGui::GetIO();
    }

    void ImGuiBackend::createImGuiVulkanContext(Window &window, const Context &context, const Swapchain &swapchain,
                                                vk::SampleCountFlagBits sampleCount)
    {
        ImGui_ImplGlfw_InitForVulkan(window.getGLFWWindow(), true);

        ImGui_ImplVulkan_InitInfo initInfo{};

        initInfo.Instance = *context.m_Instance;
        initInfo.PhysicalDevice = *context.m_PhysicalDevice;
        initInfo.Device = *context.m_Device;
        initInfo.QueueFamily = context.getGraphicsQueueFamilyIndex();
        initInfo.Queue = *context.m_GraphicsQueue;
        initInfo.DescriptorPool = *m_DescriptorPool;
        initInfo.MinImageCount = swapchain.getImageCount();
        initInfo.ImageCount = swapchain.getImageCount();
        initInfo.PipelineInfoMain.MSAASamples = static_cast<VkSampleCountFlagBits>(sampleCount);
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
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = *reinterpret_cast<const VkFormat
            *>(&depthFormat);

        if (!ImGui_ImplVulkan_Init(&initInfo))
            throw std::runtime_error("Failed to initialize ImGui for Vulkan");
    }

    void ImGuiBackend::applyStyle()
    {
        // V3 theme v1.1
        // - rlyeh, public domain
        int hue07 = 'O', alt07 = 'P', nav07 = 'Y', lit01 = 0, compact01 = 0, border01 = 1, shape0123 = 1;
        bool rounded = shape0123 == 2;

        // V3 style from ImThemes
        ImGuiStyle &style = ImGui::GetStyle();

        const float _8 = compact01 ? 4 : 8;
        const float _4 = compact01 ? 2 : 4;
        const float _2 = compact01 ? 0.5 : 1;

        style.Alpha = 1.0f;
        style.DisabledAlpha = 0.3f;

        style.WindowPadding = ImVec2(4, _8);
        style.FramePadding = ImVec2(4, _4);
        style.ItemSpacing = ImVec2(_8, _2 + _2);
        style.ItemInnerSpacing = ImVec2(4, 4);
        style.IndentSpacing = 16;
        style.ScrollbarSize = compact01 ? 12 : 18;
        style.GrabMinSize = compact01 ? 16 : 20;

        style.WindowBorderSize = border01;
        style.ChildBorderSize = border01;
        style.PopupBorderSize = border01;
        style.FrameBorderSize = 0;

        style.WindowRounding = 4;
        style.ChildRounding = 6;
        style.FrameRounding = shape0123 == 0 ? 0 : shape0123 == 1 ? 4 : 12;
        style.PopupRounding = 4;
        style.ScrollbarRounding = rounded * 8 + 4;
        style.GrabRounding = style.FrameRounding;

        style.TabBorderSize = 0;
        style.TabBarBorderSize = 2;
        style.TabBarOverlineSize = 2;
        style.TabCloseButtonMinWidthSelected = -1;
        // -1:always visible, 0:visible when hovered, >0:visible when hovered if minimum width
        style.TabCloseButtonMinWidthUnselected = -1;
        style.TabRounding = rounded;

        style.CellPadding = ImVec2(8.0f, 4.0f);

        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_Right;

        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.5f, 0.5f);
        style.SeparatorTextAlign.x = 1.00f;
        style.SeparatorTextBorderSize = 1;
        style.SeparatorTextPadding = ImVec2(0, 0);

        style.WindowMinSize = ImVec2(32.0f, 16.0f);
        style.ColumnsMinSpacing = 6.0f;

        // diamond sliders
        style.CircleTessellationMaxError = shape0123 == 3 ? 4.00f : 0.30f;

        auto lit = [&](ImVec4 hi)
        {
            float h, s, v;
            ImGui::ColorConvertRGBtoHSV(hi.x, hi.y, hi.z, h, s, v);
            ImVec4 lit = ImColor::HSV(h, s * 0.80, v * 1.00, hi.w).Value;
            return lit;
        };
        auto dim = [&](ImVec4 hi)
        {
            float h, s, v;
            ImGui::ColorConvertRGBtoHSV(hi.x, hi.y, hi.z, h, s, v);
            ImVec4 dim = ImColor::HSV(h, s, lit01 ? v * 0.65 : v * 0.65, hi.w).Value;
            if (hi.z > hi.x && hi.z > hi.y) return ImVec4(dim.x, dim.y, hi.z, dim.w);
            return dim;
        };

        const ImVec4 cyan = ImVec4(000 / 255.f, 192 / 255.f, 255 / 255.f, 1.00f);
        const ImVec4 red = ImVec4(230 / 255.f, 000 / 255.f, 000 / 255.f, 1.00f);
        const ImVec4 yellow = ImVec4(240 / 255.f, 210 / 255.f, 000 / 255.f, 1.00f);
        const ImVec4 orange = ImVec4(255 / 255.f, 144 / 255.f, 000 / 255.f, 1.00f);
        const ImVec4 lime = ImVec4(192 / 255.f, 255 / 255.f, 000 / 255.f, 1.00f);
        const ImVec4 aqua = ImVec4(000 / 255.f, 255 / 255.f, 192 / 255.f, 1.00f);
        const ImVec4 magenta = ImVec4(255 / 255.f, 000 / 255.f, 88 / 255.f, 1.00f);
        const ImVec4 purple = ImVec4(192 / 255.f, 000 / 255.f, 255 / 255.f, 1.00f);

        ImVec4 alt = cyan;
        /**/
        if (alt07 == 0 || alt07 == 'C') alt = cyan;
        else if (alt07 == 1 || alt07 == 'R') alt = red;
        else if (alt07 == 2 || alt07 == 'Y') alt = yellow;
        else if (alt07 == 3 || alt07 == 'O') alt = orange;
        else if (alt07 == 4 || alt07 == 'L') alt = lime;
        else if (alt07 == 5 || alt07 == 'A') alt = aqua;
        else if (alt07 == 6 || alt07 == 'M') alt = magenta;
        else if (alt07 == 7 || alt07 == 'P') alt = purple;
        if (lit01) alt = dim(alt);

        ImVec4 hi = cyan, lo = dim(cyan);
        /**/
        if (hue07 == 0 || hue07 == 'C') lo = dim(hi = cyan);
        else if (hue07 == 1 || hue07 == 'R') lo = dim(hi = red);
        else if (hue07 == 2 || hue07 == 'Y') lo = dim(hi = yellow);
        else if (hue07 == 3 || hue07 == 'O') lo = dim(hi = orange);
        else if (hue07 == 4 || hue07 == 'L') lo = dim(hi = lime);
        else if (hue07 == 5 || hue07 == 'A') lo = dim(hi = aqua);
        else if (hue07 == 6 || hue07 == 'M') lo = dim(hi = magenta);
        else if (hue07 == 7 || hue07 == 'P') lo = dim(hi = purple);
        //    if( lit01 ) { ImVec4 tmp = hi; hi = lo; lo = lit(tmp); }

        ImVec4 nav = orange;
        /**/
        if (nav07 == 0 || nav07 == 'C') nav = cyan;
        else if (nav07 == 1 || nav07 == 'R') nav = red;
        else if (nav07 == 2 || nav07 == 'Y') nav = yellow;
        else if (nav07 == 3 || nav07 == 'O') nav = orange;
        else if (nav07 == 4 || nav07 == 'L') nav = lime;
        else if (nav07 == 5 || nav07 == 'A') nav = aqua;
        else if (nav07 == 6 || nav07 == 'M') nav = magenta;
        else if (nav07 == 7 || nav07 == 'P') nav = purple;
        if (lit01) nav = dim(nav);

        const ImVec4
                link = ImVec4(0.26f, 0.59f, 0.98f, 1.00f),
                grey0 = ImVec4(0.04f, 0.05f, 0.07f, 1.00f),
                grey1 = ImVec4(0.08f, 0.09f, 0.11f, 1.00f),
                grey2 = ImVec4(0.10f, 0.11f, 0.13f, 1.00f),
                grey3 = ImVec4(0.12f, 0.13f, 0.15f, 1.00f),
                grey4 = ImVec4(0.16f, 0.17f, 0.19f, 1.00f),
                grey5 = ImVec4(0.18f, 0.19f, 0.21f, 1.00f);

#define Luma(v,a) ImVec4((v)/100.f,(v)/100.f,(v)/100.f,(a)/100.f)

        style.Colors[ImGuiCol_Text] = Luma(100, 100);
        style.Colors[ImGuiCol_TextDisabled] = Luma(39, 100);
        style.Colors[ImGuiCol_WindowBg] = grey1;
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
        style.Colors[ImGuiCol_PopupBg] = grey1;
        style.Colors[ImGuiCol_Border] = grey4;
        style.Colors[ImGuiCol_BorderShadow] = grey1;
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.13f, 0.15f, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered] = grey4;
        style.Colors[ImGuiCol_FrameBgActive] = grey4;
        style.Colors[ImGuiCol_TitleBg] = grey0;
        style.Colors[ImGuiCol_TitleBgActive] = grey0;
        style.Colors[ImGuiCol_TitleBgCollapsed] = grey1;
        style.Colors[ImGuiCol_MenuBarBg] = grey2;
        style.Colors[ImGuiCol_ScrollbarBg] = grey0;
        style.Colors[ImGuiCol_ScrollbarGrab] = grey3;
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = lo;
        style.Colors[ImGuiCol_ScrollbarGrabActive] = hi;
        style.Colors[ImGuiCol_CheckMark] = alt;
        style.Colors[ImGuiCol_SliderGrab] = lo;
        style.Colors[ImGuiCol_SliderGrabActive] = hi;
        style.Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
        style.Colors[ImGuiCol_ButtonHovered] = lo;
        style.Colors[ImGuiCol_ButtonActive] = grey5;
        style.Colors[ImGuiCol_Header] = grey3;
        style.Colors[ImGuiCol_HeaderHovered] = lo;
        style.Colors[ImGuiCol_HeaderActive] = hi;
        style.Colors[ImGuiCol_Separator] = ImVec4(0.13f, 0.15f, 0.19f, 1.00f);
        style.Colors[ImGuiCol_SeparatorHovered] = lo;
        style.Colors[ImGuiCol_SeparatorActive] = hi;
        style.Colors[ImGuiCol_ResizeGrip] = Luma(15, 100);
        style.Colors[ImGuiCol_ResizeGripHovered] = lo;
        style.Colors[ImGuiCol_ResizeGripActive] = hi;
        style.Colors[ImGuiCol_InputTextCursor] = Luma(100, 100);
        style.Colors[ImGuiCol_TabHovered] = grey3;
        style.Colors[ImGuiCol_Tab] = grey1;
        style.Colors[ImGuiCol_TabSelected] = grey3;
        style.Colors[ImGuiCol_TabSelectedOverline] = hi;
        style.Colors[ImGuiCol_TabDimmed] = grey1;
        style.Colors[ImGuiCol_TabDimmedSelected] = grey1;
        style.Colors[ImGuiCol_TabDimmedSelectedOverline] = lo;
        style.Colors[ImGuiCol_PlotLines] = grey5;
        style.Colors[ImGuiCol_PlotLinesHovered] = lo;
        style.Colors[ImGuiCol_PlotHistogram] = grey5;
        style.Colors[ImGuiCol_PlotHistogramHovered] = lo;
        style.Colors[ImGuiCol_TableHeaderBg] = grey0;
        style.Colors[ImGuiCol_TableBorderStrong] = grey0;
        style.Colors[ImGuiCol_TableBorderLight] = grey0;
        style.Colors[ImGuiCol_TableRowBg] = grey3;
        style.Colors[ImGuiCol_TableRowBgAlt] = grey2;
        style.Colors[ImGuiCol_TextLink] = link;
        style.Colors[ImGuiCol_TextSelectedBg] = Luma(39, 100);
        style.Colors[ImGuiCol_TreeLines] = Luma(39, 100);
        style.Colors[ImGuiCol_DragDropTarget] = nav;
        style.Colors[ImGuiCol_NavCursor] = nav;
        style.Colors[ImGuiCol_NavWindowingHighlight] = lo;
        style.Colors[ImGuiCol_NavWindowingDimBg] = Luma(0, 63);
        style.Colors[ImGuiCol_ModalWindowDimBg] = Luma(0, 63);

        if (lit01)
        {
            for (int i = 0; i < ImGuiCol_COUNT; i++)
            {
                float H, S, V;
                ImVec4 &col = style.Colors[i];
                ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);
                if (S < 0.5) V = 1.0 - V, S *= 0.15;
                ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
            }
        }

#undef Luma
}

}
