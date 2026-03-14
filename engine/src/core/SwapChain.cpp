#include "SwapChain.h"

#include "Logger.h"

namespace kailux
{
    SwapChain::SwapChain() : m_SwapChain(nullptr), m_ImageFormat(), m_Extent({}), m_SurfaceFormat({}),
                             m_SemaphoreIndex(0)
    {
    }

    SwapChain::SwapChain(SwapChain &&other) noexcept : m_SwapChain(std::move(other.m_SwapChain)),
                                                       m_Images(std::move(other.m_Images)),
                                                       m_ImageViews(std::move(other.m_ImageViews)),
                                                       m_ImageFormat(other.m_ImageFormat),
                                                       m_Extent(other.m_Extent),
                                                       m_SurfaceFormat(other.m_SurfaceFormat),
                                                       m_AcquireSemaphores(std::move(other.m_AcquireSemaphores)),
                                                       m_PresentSemaphores(std::move(other.m_PresentSemaphores)),
                                                       m_SemaphoreIndex(0)
    {
    }

    SwapChain &SwapChain::operator=(SwapChain &&other) noexcept
    {
        if (this != &other)
        {
            m_SwapChain = std::move(other.m_SwapChain);
            m_Images = std::move(other.m_Images);
            m_ImageViews = std::move(other.m_ImageViews);
            m_ImageFormat = other.m_ImageFormat;
            m_Extent = other.m_Extent;
            m_SurfaceFormat = other.m_SurfaceFormat;
            m_AcquireSemaphores = std::move(other.m_AcquireSemaphores);
            m_PresentSemaphores = std::move(other.m_PresentSemaphores);
            m_SemaphoreIndex = other.m_SemaphoreIndex;
        }
        return *this;
    }

    void SwapChain::createSwapChain(Window &window, const Context &context)
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
        swapChainCreateInfo.oldSwapchain = *m_SwapChain;

        m_SwapChain = vk::raii::SwapchainKHR(context.m_Device, swapChainCreateInfo);

        m_Images = m_SwapChain.getImages();
    }

    SwapChain SwapChain::create(Window &window, const Context &context)
    {
        KAILUX_LOG_PARENT_CLR_CYAN("[SWAPCHAIN]")
        SwapChain swapChain;

        swapChain.createSwapChain(window, context);
        KAILUX_LOG_CHILD_CLR_CYAN("Swap chain created")

        swapChain.createImageViews(context);
        KAILUX_LOG_CHILD_CLR_CYAN("Image views created")

        swapChain.createSyncObjects(context);
        KAILUX_LOG_CHILD_CLR_CYAN("Semaphores created")

        return swapChain;
    }

    void SwapChain::recreate(Window &window, const Context &context)
    {
        int width = 0, height = 0;
        while (width == 0 || height == 0)
        {
            window.getFramebufferSize(width, height);
            window.waitForEvents();
        }

        context.getDevice().waitIdle();

        m_ImageViews.clear();
        m_Images.clear();
        m_AcquireSemaphores.clear();
        m_PresentSemaphores.clear();
        m_SemaphoreIndex = 0;

        createSwapChain(window, context);
        createImageViews(context);
        createSyncObjects(context);
    }

    vk::Format SwapChain::getFormat() const
    {
        return m_ImageFormat;
    }

    vk::Extent2D SwapChain::getExtent() const
    {
        return m_Extent;
    }

    vk::Image SwapChain::getImage(uint32_t index) const
    {
        return m_Images[index];
    }

    vk::ImageView SwapChain::getImageView(uint32_t index) const
    {
        return *m_ImageViews[index];
    }

    uint32_t SwapChain::getImageCount() const
    {
        return static_cast<uint32_t>(m_Images.size());
    }

    std::optional<SwapChain::AcquireResult> SwapChain::acquire()
    {
        vk::Semaphore semaphore = *m_AcquireSemaphores[m_SemaphoreIndex];
        auto [result, imageIndex] = m_SwapChain.acquireNextImage(
            UINT64_MAX,
            semaphore,
            nullptr
        );

        if (result == vk::Result::eErrorOutOfDateKHR)
            return std::nullopt;

        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
            throw std::runtime_error("Failed to acquire swap chain image");

        m_SemaphoreIndex = (m_SemaphoreIndex + 1) % m_AcquireSemaphores.size();

        return AcquireResult(imageIndex, semaphore);
    }

    vk::Semaphore SwapChain::getPresentSemaphore(uint32_t index) const
    {
        return *m_PresentSemaphores[index];
    }

    bool SwapChain::present(const Context &context, uint32_t imageIndex, vk::Semaphore renderFinishedSemaphore) const
    {
        vk::PresentInfoKHR presentInfo{
            renderFinishedSemaphore,
            *m_SwapChain,
            imageIndex
        };

        vk::Result result = context.getGraphicsQueue().presentKHR(presentInfo);

        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
            return false;

        if (result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to present swap chain image");

        return true;
    }

    vk::SurfaceFormatKHR SwapChain::choose_swap_surface_format(
        const std::vector<vk::SurfaceFormatKHR> &availableFormats)
    {
        for (auto format: availableFormats)
            if (format.format == vk::Format::eB8G8R8A8Unorm &&
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                return format;

        return availableFormats[0];
    }

    vk::Extent2D SwapChain::choose_swap_extent(const vk::SurfaceCapabilitiesKHR &capabilities, Window &window)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;

        int width, height;

        glfwGetFramebufferSize(window.getGLFWWindow(), &width, &height);

        return {
            std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
    }

    vk::PresentModeKHR SwapChain::choose_swap_present_mode(const std::vector<vk::PresentModeKHR> &availablePresentModes)
    {
        // for (auto presentMode: availablePresentModes)
        //     if (presentMode == vk::PresentModeKHR::eMailbox)
        //         return presentMode;

        return vk::PresentModeKHR::eFifo;
    }

    void SwapChain::createImageViews(const Context &context)
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

    void SwapChain::createSyncObjects(const Context &context)
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
