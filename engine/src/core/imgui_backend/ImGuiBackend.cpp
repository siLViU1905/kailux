#include "ImGuiBackend.h"
#include "../Logger.h"
#include "bindings/imgui_impl_glfw.h"
#include "bindings/imgui_impl_vulkan.h"
#include "../SwapChain.h"

namespace kailux
{
    ImGuiBackend::ImGuiBackend() : p_Context(nullptr), p_IO(nullptr), m_DescriptorPool({})
    {
    }

    ImGuiBackend::ImGuiBackend(ImGuiBackend &&other) noexcept : p_Context(other.p_Context),
                                                                p_IO(other.p_IO), m_DescriptorPool(std::move(other.m_DescriptorPool))
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

    ImGuiBackend ImGuiBackend::create(Window& window, const Context& context, const SwapChain& swapchain)
    {
        KAILUX_LOG_PARENT_CLR_MAGENTA("[IMGUI BACKEND]")
        ImGuiBackend imguiBackend;

        imguiBackend.createImGuiContext();
        KAILUX_LOG_CHILD_CLR_MAGENTA("ImGui context created")

        imguiBackend.createDescriptorPool(context);
        KAILUX_LOG_CHILD_CLR_MAGENTA("Descriptor pool created")

        imguiBackend.createImGuiVulkanContext(window, context, swapchain);
        KAILUX_LOG_CHILD_CLR_MAGENTA("ImGui Vulkan context created")

        return imguiBackend;
    }

    void ImGuiBackend::beginFrame()
    {
        ImGui_ImplVulkan_NewFrame();

        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();
    }

    void ImGuiBackend::endFrame()
    {
        ImGui::Render();
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

    void ImGuiBackend::createImGuiVulkanContext(Window& window, const Context& context, const SwapChain& swapchain)
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
        //no MSAA for now
        //initInfo.PipelineInfoMain.MSAASamples;
        initInfo.Allocator = nullptr;
        initInfo.CheckVkResultFn = nullptr;

        initInfo.UseDynamicRendering = true;
        initInfo.PipelineInfoMain.RenderPass = VK_NULL_HANDLE;

        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;

        auto colorFormat = swapchain.getFormat();
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = reinterpret_cast<const VkFormat*>(&colorFormat);
        auto depthFormat = swapchain.getDepthFormat();
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = *reinterpret_cast<const VkFormat*>(&depthFormat);

        if (!ImGui_ImplVulkan_Init(&initInfo))
            throw std::runtime_error("Failed to initialize ImGui for Vulkan");
    }
}
