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
                                                 mTransferQueueFamilyIndex(other.mTransferQueueFamilyIndex),
                                                 mDeviceInfo(std::move(other.mDeviceInfo))
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
            mDeviceInfo = std::move(other.mDeviceInfo);
        }
        return *this;
    }

    Context Context::create(Window &window)
    {
        log::console.debug("context: creating");
        Context context;

        context.createInstance();
        log::console.debug("context: instance created");

        context.setupDebugMessenger();
        log::console.debug("context: debug messenger created");

        context.createSurface(window);
        log::console.debug("context: surface created");

        context.pickPhysicalDevice();
        log::console.debug("context: suitable physical device found");

        context.createLogicalDevice();
        log::console.debug("context: logical device created");

        context.createQueues();
        log::console.debug("context: queues created");

        log::console.debug("context: transfer queue: dedicated={}, family={}",
            context.hasDedicatedTransferQueue(), context.mTransferQueueFamilyIndex);

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

    DeviceInfo Context::getDeviceInfo() const
    {
        return extract_device_info(mPhysicalDevice);
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
        auto typeStr{to_string(type)};
        std::string messageStr{pCallbackData->pMessage};

        switch (severity)
        {
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
                log::console.debug("vulkan validation layer: type {} msg {}", typeStr, messageStr);
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
                log::console.info("vulkan validation layer: type {} msg {}", typeStr, messageStr);
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
                log::console.warning("vulkan validation layer: type {} msg {}", typeStr, messageStr);
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
                log::console.error("vulkan validation layer: type {} msg {}", typeStr, messageStr);
                break;
            default:
                log::console.info("vulkan validation layer: type {} msg {}", typeStr, messageStr);
                break;
        }

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
        auto devices = mInstance.enumeratePhysicalDevices();
        if (devices.empty())
            throw std::runtime_error("Failed to find GPUs with Vulkan support");

        auto candidates = devices | std::views::filter(is_device_suitable);

        auto best = std::ranges::max_element(candidates, {}, score_device);
        if (best == candidates.end())
            throw std::runtime_error("Failed to find a suitable GPU");

        mPhysicalDevice = *best;
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

    bool Context::is_device_suitable(const vk::raii::PhysicalDevice &device)
    {
        if (device.getProperties().apiVersion < vk::ApiVersion13)
            return false;

        if (!device.getFeatures().geometryShader)
            return false;

        auto families = device.getQueueFamilyProperties();
        bool hasGraphics = std::ranges::any_of(families, [](const auto& p)
        {
            return static_cast<bool>(p.queueFlags & vk::QueueFlagBits::eGraphics);
        });
        if (!hasGraphics)
            return false;

        const auto available = device.enumerateDeviceExtensionProperties();
        return std::ranges::all_of(kDeviceExtensions, [&](const auto required)
        {
            return std::ranges::any_of(available, [&](const auto& ext)
            {
                return std::string_view(ext.extensionName) == required;
            });
        });
    }

    uint32_t Context::score_device(const vk::raii::PhysicalDevice &device)
    {
        auto props = device.getProperties();
        uint32_t score = props.limits.maxImageDimension2D;
        if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            score += 1000;
        return score;
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

    DeviceInfo Context::extract_device_info(const vk::raii::PhysicalDevice &device)
    {
        auto chain{device.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceDriverProperties>()};

        DeviceInfo info;
        auto props{chain.get<vk::PhysicalDeviceProperties2>().properties};
        info.deviceName = props.deviceName.data();

        auto driver{chain.get<vk::PhysicalDeviceDriverProperties>()};
        info.driverName = driver.driverName.data();
        info.driverInfo = driver.driverInfo.data();

        auto mem = device.getMemoryProperties();
        vk::DeviceSize vram = 0;
        for (uint32_t i = 0; i < mem.memoryHeapCount; ++i)
            if (mem.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
                vram += mem.memoryHeaps[i].size;

        info.vramSizeMB = static_cast<uint32_t>(vram / (1024 * 1024));

        auto extensions = device.enumerateDeviceExtensionProperties();
        for (const auto& extension : extensions)
            info.extensions.emplace_back(extension.extensionName);

        return info;
    }
}
