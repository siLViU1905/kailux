#include "Context.h"

#include <map>
#include <GLFW/glfw3.h>
#include <print>

#include "Log.h"

namespace kailux
{
    Context::Context() : mContext({}), mInstance({}), mDebugMessenger({}), mPhysicalDevice({}), mDevice({}),
                         mGraphicsQueue({}), mTransferQueue({}), mSurface({}), mGraphicsQueueFamilyIndex(~0), mTransferQueueFamilyIndex(~0)
    {
    }

    Context::Context(Context &&other) noexcept : mContext(std::move(other.mContext)),
                                                 mInstance(std::move(other.mInstance)),
                                                 mDebugMessenger(std::move(other.mDebugMessenger)),
                                                 mPhysicalDevice(std::move(other.mPhysicalDevice)),
                                                 mDevice(std::move(other.mDevice)),
                                                 mGraphicsQueue(std::move(other.mGraphicsQueue)),
                                                 mTransferQueue(std::move(other.mTransferQueue)),
                                                 mSurface(std::move(other.mSurface)),
                                                 mGraphicsQueueFamilyIndex(other.mGraphicsQueueFamilyIndex),
                                                 mTransferQueueFamilyIndex(other.mTransferQueueFamilyIndex)
    {
    }

    Context &Context::operator=(Context &&other) noexcept
    {
        if (this != &other)
        {
            mContext = std::move(other.mContext);
            mInstance = std::move(other.mInstance);
            mDebugMessenger = std::move(other.mDebugMessenger);
            mPhysicalDevice = std::move(other.mPhysicalDevice);
            mDevice = std::move(other.mDevice);
            mGraphicsQueue = std::move(other.mGraphicsQueue);
            mTransferQueue = std::move(other.mTransferQueue);
            mSurface = std::move(other.mSurface);
            mGraphicsQueueFamilyIndex = other.mGraphicsQueueFamilyIndex;
            mTransferQueueFamilyIndex = other.mTransferQueueFamilyIndex;
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
            context.hasDedicatedTransferQueue(), context.mTransferQueueFamilyIndex))

        return context;
    }

    vk::PhysicalDevice Context::getPhysicalDevice() const
    {
        return *mPhysicalDevice;
    }

    vk::Device Context::getDevice() const
    {
        return *mDevice;
    }

    vk::SurfaceKHR Context::getSurface() const
    {
        return *mSurface;
    }

    vk::Queue Context::getGraphicsQueue() const
    {
        return *mGraphicsQueue;
    }

    vk::Queue Context::getTransferQueue() const
    {
        return *mTransferQueue;
    }

    uint32_t Context::getGraphicsQueueFamilyIndex() const
    {
        return mGraphicsQueueFamilyIndex;
    }

    uint32_t Context::getTransferQueueFamilyIndex() const
    {
        return mTransferQueueFamilyIndex;
    }

    uint32_t Context::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
    {
        auto memProperties = mPhysicalDevice.getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;

        throw std::runtime_error("Failed to find suitable memory type");
    }

    vk::SampleCountFlagBits Context::getMaxUsableSampleCount() const
    {
        auto props = mPhysicalDevice.getProperties();

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
        return mTransferQueueFamilyIndex != mGraphicsQueueFamilyIndex;
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

        auto layerProperties = mContext.enumerateInstanceLayerProperties();

        for (auto const &requiredLayer: requiredLayers)

            if (std::ranges::none_of(layerProperties,
                                     [requiredLayer](auto const &layerProperty)
                                     {
                                         return std::strcmp(layerProperty.layerName, requiredLayer) == 0;
                                     }))
                throw std::runtime_error("Required layer not supported: " + std::string(requiredLayer));


        auto requiredExtensions = get_required_extensions();

        auto extensionProperties = mContext.enumerateInstanceExtensionProperties();
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

        mInstance = vk::raii::Instance(mContext, createInfo);
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

        mDebugMessenger = mInstance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    }

    void Context::createSurface(Window &window)
    {
        VkSurfaceKHR surface;

        if (glfwCreateWindowSurface(*mInstance, window.getGLFWWindow(), nullptr, &surface))
            throw std::runtime_error("failed to create window surface");

        mSurface = vk::raii::SurfaceKHR(mInstance, surface);
    }

    void Context::pickPhysicalDevice()
    {
        auto devices = vk::raii::PhysicalDevices(mInstance);

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
                                                          mPhysicalDevice = device;

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
            mPhysicalDevice = std::move(candidates.rbegin()->second);
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

        auto qIndex = find_graphics_family(mPhysicalDevice, mSurface);
        if (!qIndex)
            throw std::runtime_error("Could not find a queue for graphics and present -> terminating");

        mGraphicsQueueFamilyIndex = *qIndex;
        queueInfos.emplace_back(vk::DeviceQueueCreateFlags{}, mGraphicsQueueFamilyIndex, 1, &queuePriority);

        qIndex = find_transfer_family(mPhysicalDevice);
        if (qIndex)
        {
            mTransferQueueFamilyIndex = *qIndex;
            queueInfos.emplace_back(vk::DeviceQueueCreateFlags{}, mTransferQueueFamilyIndex, 1, &queuePriority);
        }

        vk::DeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.pNext = &f2;

        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueInfos.data();
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

        mDevice = vk::raii::Device(mPhysicalDevice, deviceCreateInfo);
    }

    void Context::createQueues()
    {
        mGraphicsQueue = vk::raii::Queue(mDevice, mGraphicsQueueFamilyIndex, 0);

        if (mTransferQueueFamilyIndex == ~0u)
            mTransferQueueFamilyIndex = mGraphicsQueueFamilyIndex;

        mTransferQueue = vk::raii::Queue(mDevice, mTransferQueueFamilyIndex, 0);
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
