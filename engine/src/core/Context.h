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
        uint32_t           getGraphicsQueueFamilyIndex() const;

        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

        friend class Swapchain;
        friend class FrameData;
        friend class ImGuiBackend;

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

        vk::raii::Context                m_Context;
        vk::raii::Instance               m_Instance;
        vk::raii::DebugUtilsMessengerEXT m_DebugMessenger;
        vk::raii::PhysicalDevice         m_PhysicalDevice;
        vk::raii::Device                 m_Device;
        vk::raii::Queue                  m_GraphicsQueue;
        vk::raii::SurfaceKHR             m_Surface;
        uint32_t                         m_GraphicsQueueFamilyIndex;
        static constexpr std::array      s_ValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
            };
#ifdef NDEBUG
        static constexpr bool            s_EnableValidationLayers = false;
#else
        static constexpr bool            s_EnableValidationLayers = true;
#endif
        static constexpr std::array      s_DeviceExtensions = {
            vk::KHRSwapchainExtensionName,
            vk::KHRSpirv14ExtensionName,
            vk::KHRSynchronization2ExtensionName,
            vk::KHRCreateRenderpass2ExtensionName,
            vk::KHRMaintenance7ExtensionName
        };
    };
}
