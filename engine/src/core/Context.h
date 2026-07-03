#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "window/Window.h"

namespace kailux
{
    class Context
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Context)

        static Context create(Window& window);

        vk::PhysicalDevice getPhysicalDevice() const;
        vk::Device         getDevice() const;
        vk::SurfaceKHR     getSurface() const;
        vk::Queue          getGraphicsQueue() const;
        vk::Queue          getTransferQueue() const;
        uint32_t           getGraphicsQueueFamilyIndex() const;
        uint32_t           getTransferQueueFamilyIndex() const;

        uint32_t                findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
        vk::SampleCountFlagBits getMaxUsableSampleCount() const;
        bool                    hasDedicatedTransferQueue() const;

        friend class Swapchain;
        friend class FrameData;
        friend class ImGuiBackend;
        friend class BufferAllocator;
        friend class TextureAllocator;
        friend class Pipeline;
        friend class DescriptorLayout;
        friend class DescriptorPool;
        friend class DescriptorSet;
        friend class OneTimeCommand;
        friend class TransferManager;

    private:
        static std::vector<const char *> get_required_extensions();
        static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                              vk::DebugUtilsMessageTypeFlagsEXT type,
                                                              const vk::DebugUtilsMessengerCallbackDataEXT *
                                                              pCallbackData,
                                                              void *pUser);

        void createInstance();
        void setupDebugMessenger();
        void createSurface(Window& window);
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createQueues();

        static std::optional<uint32_t> find_graphics_family(const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR &surface);
        static std::optional<uint32_t> find_transfer_family(const vk::raii::PhysicalDevice& device);

        vk::raii::Context                m_Context;
        vk::raii::Instance               m_Instance;
        vk::raii::DebugUtilsMessengerEXT m_DebugMessenger;
        vk::raii::PhysicalDevice         m_PhysicalDevice;
        vk::raii::Device                 m_Device;
        vk::raii::Queue                  m_GraphicsQueue;
        vk::raii::Queue                  m_TransferQueue;
        vk::raii::SurfaceKHR             m_Surface;
        uint32_t                         m_GraphicsQueueFamilyIndex;
        uint32_t                         m_TransferQueueFamilyIndex;
        static constexpr std::array      kValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
            };
#ifdef NDEBUG
        static constexpr bool            kEnableValidationLayers = false;
#else
        static constexpr bool            kEnableValidationLayers = true;
#endif
        static constexpr std::array      kDeviceExtensions = {
            vk::KHRSwapchainExtensionName,
            vk::KHRPresentModeFifoLatestReadyExtensionName,
            vk::KHRSpirv14ExtensionName,
            vk::KHRSynchronization2ExtensionName,
            vk::KHRCreateRenderpass2ExtensionName,
            vk::KHRMaintenance7ExtensionName
        };
    };
}
