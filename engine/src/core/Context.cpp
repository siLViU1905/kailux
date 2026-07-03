#include "Context.h"

#include <map>
#include <GLFW/glfw3.h>
#include <print>

#include "Log.h"

namespace kailux
{
    Context::Context() : m_Context({}), m_Instance({}), m_DebugMessenger({}), m_PhysicalDevice({}), m_Device({}),
                         m_GraphicsQueue({}), m_TransferQueue({}), m_Surface({}), m_GraphicsQueueFamilyIndex(~0), m_TransferQueueFamilyIndex(~0)
    {
    }

    Context::Context(Context &&other) noexcept : m_Context(std::move(other.m_Context)),
                                                 m_Instance(std::move(other.m_Instance)),
                                                 m_DebugMessenger(std::move(other.m_DebugMessenger)),
                                                 m_PhysicalDevice(std::move(other.m_PhysicalDevice)),
                                                 m_Device(std::move(other.m_Device)),
                                                 m_GraphicsQueue(std::move(other.m_GraphicsQueue)),
                                                 m_TransferQueue(std::move(other.m_TransferQueue)),
                                                 m_Surface(std::move(other.m_Surface)),
                                                 m_GraphicsQueueFamilyIndex(other.m_GraphicsQueueFamilyIndex),
                                                 m_TransferQueueFamilyIndex(other.m_TransferQueueFamilyIndex)
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
            m_TransferQueue = std::move(other.m_TransferQueue);
            m_Surface = std::move(other.m_Surface);
            m_GraphicsQueueFamilyIndex = other.m_GraphicsQueueFamilyIndex;
            m_TransferQueueFamilyIndex = other.m_TransferQueueFamilyIndex;
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

        context.createQueues();
        KAILUX_LOG_CHILD_CLR_YELLOW("Queues created")

        KAILUX_LOG_CHILD_CLR_YELLOW(std::format("Transfer queue: dedicated={}, family={}",
            context.hasDedicatedTransferQueue(), context.m_TransferQueueFamilyIndex))

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

    vk::Queue Context::getTransferQueue() const
    {
        return *m_TransferQueue;
    }

    uint32_t Context::getGraphicsQueueFamilyIndex() const
    {
        return m_GraphicsQueueFamilyIndex;
    }

    uint32_t Context::getTransferQueueFamilyIndex() const
    {
        return m_TransferQueueFamilyIndex;
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

    vk::SampleCountFlagBits Context::getMaxUsableSampleCount() const
    {
        auto props = m_PhysicalDevice.getProperties();

        vk::SampleCountFlags counts = props.limits.framebufferColorSampleCounts &
                                      props.limits.framebufferDepthSampleCounts;

        if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
        if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
        if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
        if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
        if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
        if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

        return vk::SampleCountFlagBits::e1;
    }

    bool Context::hasDedicatedTransferQueue() const
    {
        return m_TransferQueueFamilyIndex != m_GraphicsQueueFamilyIndex;
    }

    std::vector<const char *> Context::get_required_extensions()
    {
        uint32_t glfwExtensionCount = 0;

        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (kEnableValidationLayers)
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

        if (kEnableValidationLayers)
            requiredLayers.assign(kValidationLayers.begin(), kValidationLayers.end());

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
        if constexpr (!kEnableValidationLayers)
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
                                                      for (auto const &extension: kDeviceExtensions)
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
        vk::StructureChain<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan12Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
            vk::PhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT,
            vk::PhysicalDeviceMaintenance7FeaturesKHR,
            vk::PhysicalDevicePresentModeFifoLatestReadyFeaturesEXT
        > featureChain;

        auto &f2 = featureChain.get<vk::PhysicalDeviceFeatures2>();
        auto &f11 = featureChain.get<vk::PhysicalDeviceVulkan11Features>();
        auto &f12 = featureChain.get<vk::PhysicalDeviceVulkan12Features>();
        auto &f13 = featureChain.get<vk::PhysicalDeviceVulkan13Features>();
        auto &fExt = featureChain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        auto &fUnusedAtt = featureChain.get<vk::PhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT>();
        auto &fMaint7 = featureChain.get<vk::PhysicalDeviceMaintenance7FeaturesKHR>();
        auto &fFifoLatest = featureChain.get<vk::PhysicalDevicePresentModeFifoLatestReadyFeaturesEXT>();

        f2.features.samplerAnisotropy = vk::True;
        f2.features.sampleRateShading = vk::True;
        f2.features.multiDrawIndirect = vk::True;
        f2.features.independentBlend  = vk::True;

        f11.shaderDrawParameters = vk::True;

        f12.runtimeDescriptorArray = vk::True;
        f12.descriptorBindingPartiallyBound = vk::True;
        f12.descriptorBindingSampledImageUpdateAfterBind = vk::True;
        f12.descriptorBindingUniformBufferUpdateAfterBind = vk::True;
        f12.descriptorBindingStorageBufferUpdateAfterBind = vk::True;
        f12.shaderSampledImageArrayNonUniformIndexing = vk::True;
        f12.descriptorIndexing = vk::True;
        f12.drawIndirectCount = vk::True;

        f13.dynamicRendering = vk::True;
        f13.synchronization2 = vk::True;

        fExt.extendedDynamicState = vk::True;

        fUnusedAtt.dynamicRenderingUnusedAttachments = vk::True;

        fMaint7.maintenance7 = vk::True;

        fFifoLatest.presentModeFifoLatestReady = vk::True;

        std::vector<vk::DeviceQueueCreateInfo> queueInfos;
        float queuePriority{};

        auto qIndex = find_graphics_family(m_PhysicalDevice, m_Surface);
        if (!qIndex)
            throw std::runtime_error("Could not find a queue for graphics and present -> terminating");

        m_GraphicsQueueFamilyIndex = *qIndex;
        queueInfos.emplace_back(vk::DeviceQueueCreateFlags{}, m_GraphicsQueueFamilyIndex, 1, &queuePriority);

        qIndex = find_transfer_family(m_PhysicalDevice);
        if (qIndex)
        {
            m_TransferQueueFamilyIndex = *qIndex;
            queueInfos.emplace_back(vk::DeviceQueueCreateFlags{}, m_TransferQueueFamilyIndex, 1, &queuePriority);
        }

        vk::DeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.pNext = &f2;

        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueInfos.data();
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

        m_Device = vk::raii::Device(m_PhysicalDevice, deviceCreateInfo);
    }

    void Context::createQueues()
    {
        m_GraphicsQueue = vk::raii::Queue(m_Device, m_GraphicsQueueFamilyIndex, 0);

        if (m_TransferQueueFamilyIndex == ~0u)
            m_TransferQueueFamilyIndex = m_GraphicsQueueFamilyIndex;

        m_TransferQueue = vk::raii::Queue(m_Device, m_TransferQueueFamilyIndex, 0);
    }

    std::optional<uint32_t> Context::find_graphics_family(const vk::raii::PhysicalDevice &device, const vk::raii::SurfaceKHR &surface)
    {
        auto families = device.getQueueFamilyProperties();
        uint32_t i{};
        for (const auto& family : families)
        {
            bool hasGraphics = static_cast<bool>(family.queueFlags & vk::QueueFlagBits::eGraphics);
            if (hasGraphics && device.getSurfaceSupportKHR(i, surface))
                return i;
            ++i;
        }
        return std::nullopt;
    }

    std::optional<uint32_t> Context::find_transfer_family(const vk::raii::PhysicalDevice &device)
    {
        auto families = device.getQueueFamilyProperties();
        uint32_t i{};
        for (const auto& family : families)
        {
            bool hasTransfer = static_cast<bool>(family.queueFlags & vk::QueueFlagBits::eTransfer);
            bool hasGraphics = static_cast<bool>(family.queueFlags & vk::QueueFlagBits::eGraphics);
            if (hasTransfer && !hasGraphics)
                return i;
            ++i;
        }
        return std::nullopt;
    }
}
