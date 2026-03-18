#include "Context.h"

#include <map>
#include <GLFW/glfw3.h>
#include <print>

#include "Logger.h"

namespace kailux
{
    Context::Context() : m_Context({}), m_Instance({}), m_DebugMessenger({}), m_PhysicalDevice({}), m_Device({}),
                         m_GraphicsQueue({}), m_Surface({}), m_GraphicsQueueFamilyIndex(~0)
    {
    }

    Context::Context(Context &&other) noexcept : m_Context(std::move(other.m_Context)),
                                                 m_Instance(std::move(other.m_Instance)),
                                                 m_DebugMessenger(std::move(other.m_DebugMessenger)),
                                                 m_PhysicalDevice(std::move(other.m_PhysicalDevice)),
                                                 m_Device(std::move(other.m_Device)),
                                                 m_GraphicsQueue(std::move(other.m_GraphicsQueue)),
                                                 m_Surface(std::move(other.m_Surface)),
                                                 m_GraphicsQueueFamilyIndex(~0)
    {
    }

    Context &Context::operator=(Context &&other) noexcept
    {
        if (this != &other)
        {
            m_Context = std::move(other.m_Context);
            m_Instance = std::move(other.m_Instance);
            m_DebugMessenger = std::move(other.m_DebugMessenger);
            m_PhysicalDevice = std::move(other.m_PhysicalDevice);
            m_Device = std::move(other.m_Device);
            m_GraphicsQueue = std::move(other.m_GraphicsQueue);
            m_Surface = std::move(other.m_Surface);
            m_GraphicsQueueFamilyIndex = other.m_GraphicsQueueFamilyIndex;
        }
        return *this;
    }

    Context Context::create(Window &window)
    {
        KAILUX_LOG_PARENT_CLR_YELLOW("[CONTEXT]")
        Context context;

        context.createInstance();
        KAILUX_LOG_CHILD_CLR_YELLOW("Instance created")

        context.setupDebugMessenger();
        KAILUX_LOG_CHILD_CLR_YELLOW("Debug messenger created")

        context.createSurface(window);
        KAILUX_LOG_CHILD_CLR_YELLOW("Surface created")

        context.pickPhysicalDevice();
        KAILUX_LOG_CHILD_CLR_YELLOW("Suitable physical device found")

        context.createLogicalDevice();
        KAILUX_LOG_CHILD_CLR_YELLOW("Logical device created")

        return context;
    }

    vk::PhysicalDevice Context::getPhysicalDevice() const
    {
        return *m_PhysicalDevice;
    }

    vk::Device Context::getDevice() const
    {
        return *m_Device;
    }

    vk::SurfaceKHR Context::getSurface() const
    {
        return *m_Surface;
    }

    vk::Queue Context::getGraphicsQueue() const
    {
        return *m_GraphicsQueue;
    }

    uint32_t Context::getGraphicsQueueFamilyIndex() const
    {
        return m_GraphicsQueueFamilyIndex;
    }

    uint32_t Context::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
    {
        auto memProperties = m_PhysicalDevice.getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;

        throw std::runtime_error("Failed to find suitable memory type");
    }

    std::vector<const char *> Context::get_required_extensions()
    {
        uint32_t glfwExtensionCount = 0;

        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (s_EnableValidationLayers)
            extensions.push_back(vk::EXTDebugUtilsExtensionName);

        return extensions;
    }

    vk::Bool32 Context::debug_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                       vk::DebugUtilsMessageTypeFlagsEXT type,
                                       const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                       void *pUser)
    {
        std::println("Validation layer: type {} msg {}", to_string(type), pCallbackData->pMessage);

        return VK_FALSE;
    }

    void Context::createInstance()
    {
        constexpr vk::ApplicationInfo appInfo(
            "kailux",
            VK_MAKE_VERSION(1, 0, 0),
            "kailux_engine",
            VK_MAKE_VERSION(1, 0, 0),
            VK_API_VERSION_1_4
        );

        std::vector<const char *> requiredLayers;

        if (s_EnableValidationLayers)
            requiredLayers.assign(s_ValidationLayers.begin(), s_ValidationLayers.end());

        auto layerProperties = m_Context.enumerateInstanceLayerProperties();

        for (auto const &requiredLayer: requiredLayers)

            if (std::ranges::none_of(layerProperties,
                                     [requiredLayer](auto const &layerProperty)
                                     {
                                         return std::strcmp(layerProperty.layerName, requiredLayer) == 0;
                                     }))
                throw std::runtime_error("Required layer not supported: " + std::string(requiredLayer));


        auto requiredExtensions = get_required_extensions();

        auto extensionProperties = m_Context.enumerateInstanceExtensionProperties();
        for (auto const &requiredExtension: requiredExtensions)
            if (std::ranges::none_of(extensionProperties,
                                     [requiredExtension](auto const &extensionProperty)
                                     {
                                         return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
                                     }))
                throw std::runtime_error("Required extension not supported: " + std::string(requiredExtension));

        vk::InstanceCreateInfo createInfo{
            {},
            &appInfo,
            static_cast<uint32_t>(requiredLayers.size()),
            requiredLayers.data(),
            static_cast<uint32_t>(requiredExtensions.size()),
            requiredExtensions.data()
        };

        m_Instance = vk::raii::Instance(m_Context, createInfo);
    }

    void Context::setupDebugMessenger()
    {
        if (!s_EnableValidationLayers)
            return;

        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
            {},
            severityFlags,
            messageTypeFlags,
            &debug_callback
        };

        m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    }

    void Context::createSurface(Window &window)
    {
        VkSurfaceKHR surface;

        if (glfwCreateWindowSurface(*m_Instance, window.getGLFWWindow(), nullptr, &surface))
            throw std::runtime_error("failed to create window surface");

        m_Surface = vk::raii::SurfaceKHR(m_Instance, surface);
    }

    void Context::pickPhysicalDevice()
    {
        auto devices = vk::raii::PhysicalDevices(m_Instance);

        if (devices.empty())
            throw std::runtime_error("Failed to find GPUs with Vulkan support");

        const auto devIter = std::ranges::find_if(devices,
                                                  [&](auto const &device)
                                                  {
                                                      auto queueFamilies = device.getQueueFamilyProperties();
                                                      bool isSuitable =
                                                              device.getProperties().apiVersion >= VK_API_VERSION_1_3;
                                                      const auto qfpIter = std::ranges::find_if(queueFamilies,
                                                          [](vk::QueueFamilyProperties const &qfp)
                                                          {
                                                              return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) !=
                                                                     static_cast<vk::QueueFlags>(0);
                                                          });
                                                      isSuitable = isSuitable && (qfpIter != queueFamilies.end());
                                                      auto extensions = device.enumerateDeviceExtensionProperties();
                                                      bool found = true;
                                                      for (auto const &extension: s_DeviceExtensions)
                                                      {
                                                          auto extensionIter = std::ranges::find_if(
                                                              extensions, [extension](auto const &ext)
                                                              {
                                                                  return strcmp(ext.extensionName, extension) == 0;
                                                              });
                                                          found = found && extensionIter != extensions.end();
                                                      }
                                                      isSuitable = isSuitable && found;
                                                      if (isSuitable)
                                                          m_PhysicalDevice = device;

                                                      return isSuitable;
                                                  });
        if (devIter == devices.end())
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        std::multimap<int, vk::raii::PhysicalDevice> candidates;

        for (const auto &device: devices)
        {
            auto deviceProperties = device.getProperties();
            auto deviceFeatures = device.getFeatures();
            uint32_t score = 0;

            if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
                score += 1000;

            score += deviceProperties.limits.maxImageDimension2D;

            if (!deviceFeatures.geometryShader)
                continue;

            candidates.insert(std::make_pair(score, device));
        }

        if (candidates.rbegin()->first > 0)
            m_PhysicalDevice = std::move(candidates.rbegin()->second);
        else
            throw std::runtime_error("Failed to find a suitable GPU");
    }

    void Context::createLogicalDevice()
    {
        auto queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();

        uint32_t queueIndex = ~0;

        for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
        {
            if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
                m_PhysicalDevice.getSurfaceSupportKHR(qfpIndex, m_Surface))
            {
                queueIndex = qfpIndex;
                break;
            }
        }

        m_GraphicsQueueFamilyIndex = queueIndex;

        if (queueIndex == ~0)
            throw std::runtime_error("Could not find a queue for graphics and present -> terminating");

        vk::PhysicalDeviceMaintenance7FeaturesKHR maintenance7Features{};
        maintenance7Features.maintenance7 = VK_TRUE;

        vk::PhysicalDeviceFeatures2 features2{};

        features2.features.samplerAnisotropy = vk::True;
        features2.features.sampleRateShading = vk::True;

        features2.features.multiDrawIndirect = vk::True;

        vk::PhysicalDeviceVulkan12Features features12{};

        features12.runtimeDescriptorArray = vk::True;

        vk::PhysicalDeviceVulkan13Features features13{};

        features13.dynamicRendering = true;
        features13.synchronization2 = true;

        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT featuresExt{};

        featuresExt.extendedDynamicState = true;


        vk::StructureChain featureChain = {
            features2,
            features12,
            features13,
            featuresExt,
            maintenance7Features
        };

        float queuePriority = 0.f;

        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
            {},
            queueIndex,
            1,
            &queuePriority
        };

        vk::DeviceCreateInfo deviceCreateInfo{};

        deviceCreateInfo.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>();
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(s_DeviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = s_DeviceExtensions.data();

        m_Device = vk::raii::Device(m_PhysicalDevice, deviceCreateInfo);

        m_GraphicsQueue = vk::raii::Queue(m_Device, queueIndex, 0);
    }
}
