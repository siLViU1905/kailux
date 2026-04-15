#include "Swapchain.h"

#include "Logger.h"

namespace kailux
{
    Swapchain::Swapchain() : m_Swapchain(nullptr),
                             m_ColorImage({}),
                             m_DepthImage({}),
                             m_ColorImageMemory({}),
                             m_DepthImageMemory({}),
                             m_ColorImageView({}),
                             m_DepthImageView({}),
                             m_ImageFormat(),
                             m_DepthFormat(),
                             m_Extent({}),
                             m_SurfaceFormat({}),
                             m_SemaphoreIndex(0)
    {
    }

    Swapchain::Swapchain(Swapchain &&other) noexcept : m_Swapchain(std::move(other.m_Swapchain)),
                                                       m_Images(std::move(other.m_Images)),
                                                       m_ColorImage(std::move(other.m_ColorImage)),
                                                       m_DepthImage(std::move(other.m_DepthImage)),
                                                       m_ColorImageMemory(std::move(other.m_ColorImageMemory)),
                                                       m_DepthImageMemory(std::move(other.m_DepthImageMemory)),
                                                       m_ImageViews(std::move(other.m_ImageViews)),
                                                       m_ColorImageView(std::move(other.m_ColorImageView)),
                                                       m_DepthImageView(std::move(other.m_DepthImageView)),
                                                       m_ImageFormat(other.m_ImageFormat),
                                                       m_DepthFormat(other.m_DepthFormat),
                                                       m_Extent(other.m_Extent),
                                                       m_SurfaceFormat(other.m_SurfaceFormat),
                                                       m_AcquireSemaphores(std::move(other.m_AcquireSemaphores)),
                                                       m_PresentSemaphores(std::move(other.m_PresentSemaphores)),
                                                       m_SemaphoreIndex(0)
    {
    }

    Swapchain &Swapchain::operator=(Swapchain &&other) noexcept
    {
        if (this != &other)
        {
            m_Swapchain = std::move(other.m_Swapchain);
            m_Images = std::move(other.m_Images);
            m_ColorImage = std::move(other.m_ColorImage);
            m_DepthImage = std::move(other.m_DepthImage);
            m_ColorImageMemory = std::move(other.m_ColorImageMemory);
            m_DepthImageMemory = std::move(other.m_DepthImageMemory);
            m_ImageViews = std::move(other.m_ImageViews);
            m_ColorImageView = std::move(other.m_ColorImageView);
            m_DepthImageView = std::move(other.m_DepthImageView);
            m_ImageFormat = other.m_ImageFormat;
            m_DepthFormat = other.m_DepthFormat;
            m_Extent = other.m_Extent;
            m_SurfaceFormat = other.m_SurfaceFormat;
            m_AcquireSemaphores = std::move(other.m_AcquireSemaphores);
            m_PresentSemaphores = std::move(other.m_PresentSemaphores);
            m_SemaphoreIndex = other.m_SemaphoreIndex;
        }
        return *this;
    }

    void Swapchain::createSwapchain(const Window &window, const Context &context)
    {
        auto surfaceCapabilities = context.getPhysicalDevice().getSurfaceCapabilitiesKHR(context.getSurface());

        m_SurfaceFormat = choose_swap_surface_format(
            context.getPhysicalDevice().getSurfaceFormatsKHR(context.getSurface()));

        m_Extent = choose_swap_extent(surfaceCapabilities, window);

        m_ImageFormat = m_SurfaceFormat.format;

        auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);

        minImageCount = (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount)
                            ? surfaceCapabilities.maxImageCount
                            : minImageCount;

        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;

        if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
            imageCount = surfaceCapabilities.maxImageCount;

        vk::SwapchainCreateInfoKHR swapChainCreateInfo{
            vk::SwapchainCreateFlagsKHR(),
            context.getSurface(),
            minImageCount,
            m_SurfaceFormat.format,
            m_SurfaceFormat.colorSpace,
            m_Extent,
            1,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::SharingMode::eExclusive
        };

        swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
        swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        swapChainCreateInfo.presentMode = choose_swap_present_mode(
            context.getPhysicalDevice().getSurfacePresentModesKHR(context.getSurface()));
        swapChainCreateInfo.clipped = true;
        swapChainCreateInfo.oldSwapchain = *m_Swapchain;

        m_Swapchain = vk::raii::SwapchainKHR(context.m_Device, swapChainCreateInfo);

        m_Images = m_Swapchain.getImages();
    }

    Swapchain Swapchain::create(Window &window, const Context &context, vk::SampleCountFlagBits sampleCount)
    {
        KAILUX_LOG_PARENT_CLR_CYAN("[SWAPCHAIN]")
        Swapchain swapChain;

        swapChain.createSwapchain(window, context);
        KAILUX_LOG_CHILD_CLR_CYAN("Swap chain created")

        swapChain.createImageViews(context);
        KAILUX_LOG_CHILD_CLR_CYAN("Image views created")

        swapChain.createColorResources(context, sampleCount);
        KAILUX_LOG_CHILD_CLR_CYAN("Color resources created")

        swapChain.createDepthResources(context, sampleCount);
        KAILUX_LOG_CHILD_CLR_CYAN("Depth resources created")

        swapChain.createSyncObjects(context);
        KAILUX_LOG_CHILD_CLR_CYAN("Semaphores created")

        return swapChain;
    }

    void Swapchain::recreate(const Window &window, const Context &context, vk::SampleCountFlagBits sampleCount)
    {
        int width = 0, height = 0;
        while (width == 0 || height == 0)
        {
            window.getFramebufferSize(width, height);
            window.waitForEvents();
        }

        context.getDevice().waitIdle();

        // createSwapchain(window, context);

        m_ImageViews.clear();
        m_Images.clear();
        m_AcquireSemaphores.clear();
        m_PresentSemaphores.clear();
        m_SemaphoreIndex = 0;

        createSwapchain(window, context);
        createImageViews(context);
        createColorResources(context, sampleCount);
        createDepthResources(context, sampleCount);
        createSyncObjects(context);
        KAILUX_LOG_INFO("[Swapchain]",
                        std::format("Recreated with extent: x:{}, y:{}", m_Extent.width, m_Extent.height))
    }

    vk::Format Swapchain::getFormat() const
    {
        return m_ImageFormat;
    }

    vk::Format Swapchain::getDepthFormat() const
    {
        return m_DepthFormat;
    }

    vk::Extent2D Swapchain::getExtent() const
    {
        return m_Extent;
    }

    vk::Image Swapchain::getImage(uint32_t index) const
    {
        return m_Images[index];
    }

    vk::Image Swapchain::getColorImage() const
    {
        return *m_ColorImage;
    }

    vk::Image Swapchain::getDepthImage() const
    {
        return *m_DepthImage;
    }

    vk::ImageView Swapchain::getImageView(uint32_t index) const
    {
        return *m_ImageViews[index];
    }

    vk::ImageView Swapchain::getColorImageView() const
    {
        return *m_ColorImageView;
    }

    vk::ImageView Swapchain::getDepthImageView() const
    {
        return *m_DepthImageView;
    }

    uint32_t Swapchain::getImageCount() const
    {
        return static_cast<uint32_t>(m_Images.size());
    }

    std::optional<Swapchain::AcquireResult> Swapchain::acquire()
    {
        auto semaphore = *m_AcquireSemaphores[m_SemaphoreIndex];
        try
        {
            auto [result, imageIndex] = m_Swapchain.acquireNextImage(UINT64_MAX, semaphore, nullptr);

            m_SemaphoreIndex = (m_SemaphoreIndex + 1) % m_AcquireSemaphores.size();

            return AcquireResult(imageIndex, semaphore);
        } catch (const vk::OutOfDateKHRError &)
        {
            return std::nullopt;
        } catch (...)
        {
            return std::nullopt;
        }
    }

    vk::Semaphore Swapchain::getPresentSemaphore(uint32_t index) const
    {
        return *m_PresentSemaphores[index];
    }

    bool Swapchain::present(const Context &context, uint32_t imageIndex, vk::Semaphore renderFinishedSemaphore) const
    {
        vk::PresentInfoKHR presentInfo{
            renderFinishedSemaphore,
            *m_Swapchain,
            imageIndex
        };

        try
        {
            auto result = context.getGraphicsQueue().presentKHR(presentInfo);
            if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
                return false;
        } catch (const vk::OutOfDateKHRError &)
        {
            return false;
        }

        return true;
    }

    vk::SurfaceFormatKHR Swapchain::choose_swap_surface_format(
        const std::vector<vk::SurfaceFormatKHR> &availableFormats)
    {
        for (auto format: availableFormats)
            if (format.format == vk::Format::eB8G8R8A8Unorm &&
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                return format;

        return availableFormats[0];
    }

    vk::Extent2D Swapchain::choose_swap_extent(const vk::SurfaceCapabilitiesKHR &capabilities, const Window &window)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;

        int width, height;
        window.getFramebufferSize(width, height);

        return {
            std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
    }

    vk::PresentModeKHR Swapchain::choose_swap_present_mode(const std::vector<vk::PresentModeKHR> &availablePresentModes)
    {
        // for (auto presentMode: availablePresentModes)
        //     if (presentMode == vk::PresentModeKHR::eMailbox)
        //         return presentMode;

        return vk::PresentModeKHR::eFifo;
    }

    vk::Format Swapchain::find_depth_format(const Context &context)
    {
        constexpr std::array candidates = {
            vk::Format::eD32Sfloat,
            vk::Format::eD32SfloatS8Uint,
            vk::Format::eD24UnormS8Uint
        };

        for (vk::Format format: candidates)
        {
            auto props = context.getPhysicalDevice().getFormatProperties(format);
            if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
                return format;
        }

        throw std::runtime_error("Couldnt find a supported depth format");
    }

    void Swapchain::createImageViews(const Context &context)
    {
        m_ImageViews.clear();
        m_ImageViews.reserve(m_Images.size());

        for (auto image: m_Images)
        {
            vk::ImageViewCreateInfo viewInfo{
                {},
                image,
                vk::ImageViewType::e2D,
                m_ImageFormat,
                {},
                vk::ImageSubresourceRange{
                    vk::ImageAspectFlagBits::eColor,
                    0, 1,
                    0, 1
                }
            };

            m_ImageViews.emplace_back(context.m_Device, viewInfo);
        }
    }

    void Swapchain::createColorResources(const Context &context, vk::SampleCountFlagBits sampleCount)
    {
        if (sampleCount == vk::SampleCountFlagBits::e1)
            return;

        vk::ImageCreateInfo imageInfo(
            {},
            vk::ImageType::e2D,
            m_ImageFormat,
            vk::Extent3D(m_Extent.width, m_Extent.height, 1),
            1,
            1,
            sampleCount,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment
        );

        m_ColorImage = vk::raii::Image(context.m_Device, imageInfo);

        auto memReqs = m_ColorImage.getMemoryRequirements();
        m_ColorImageMemory = vk::raii::DeviceMemory(context.m_Device,
                                                    {
                                                        memReqs.size,
                                                        context.findMemoryType(
                                                            memReqs.memoryTypeBits,
                                                            vk::MemoryPropertyFlagBits::eDeviceLocal)
                                                    });
        m_ColorImage.bindMemory(*m_ColorImageMemory, 0);

        m_ColorImageView = vk::raii::ImageView(context.m_Device,
                                               {
                                                   {},
                                                   *m_ColorImage,
                                                   vk::ImageViewType::e2D,
                                                   m_ImageFormat,
                                                   {},
                                                   {
                                                       vk::ImageAspectFlagBits::eColor,
                                                       0,
                                                       1,
                                                       0,
                                                       1
                                                   }
                                               });
    }

    void Swapchain::createDepthResources(const Context &context, vk::SampleCountFlagBits sampleCount)
    {
        m_DepthFormat = find_depth_format(context);

        vk::ImageCreateInfo imageInfo{
            {},
            vk::ImageType::e2D,
            m_DepthFormat,
            vk::Extent3D{m_Extent.width, m_Extent.height, 1},
            1, 1,
            sampleCount,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment
        };

        m_DepthImage = vk::raii::Image(context.m_Device, imageInfo);

        vk::MemoryRequirements memReqs = m_DepthImage.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{
            memReqs.size,
            context.findMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
        };
        m_DepthImageMemory = vk::raii::DeviceMemory(context.m_Device, allocInfo);
        m_DepthImage.bindMemory(*m_DepthImageMemory, 0);

        vk::ImageViewCreateInfo viewInfo{
            {},
            *m_DepthImage,
            vk::ImageViewType::e2D,
            m_DepthFormat,
            {},
            vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}
        };

        m_DepthImageView = vk::raii::ImageView(context.m_Device, viewInfo);
    }

    void Swapchain::createSyncObjects(const Context &context)
    {
        m_AcquireSemaphores.reserve(m_Images.size());
        m_PresentSemaphores.reserve(m_Images.size());

        for (size_t i = 0; i < m_Images.size(); i++)
        {
            m_AcquireSemaphores.emplace_back(context.m_Device, vk::SemaphoreCreateInfo{});
            m_PresentSemaphores.emplace_back(context.m_Device, vk::SemaphoreCreateInfo{});
        }
    }
}
