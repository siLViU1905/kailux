#include "SwapChain.h"

#include "Logger.h"

namespace kailux
{
    SwapChain::SwapChain() : m_SwapChain(nullptr), m_ImageFormat(), m_Extent({}), m_SurfaceFormat({})
    {
    }

    SwapChain::SwapChain(SwapChain &&other) noexcept : m_SwapChain(std::move(other.m_SwapChain)),
                                                       m_Images(std::move(other.m_Images)),
                                                       m_ImageViews(std::move(other.m_ImageViews)),
                                                       m_ImageFormat(other.m_ImageFormat),
                                                       m_Extent(other.m_Extent),
                                                       m_SurfaceFormat(other.m_SurfaceFormat)
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
        }
        return *this;
    }

    void SwapChain::createSwapChain(Window &window, Context &context)
    {
        auto surfaceCapabilities = context.m_PhysicalDevice.getSurfaceCapabilitiesKHR(context.m_Surface);

        m_SurfaceFormat = choose_swap_surface_format(context.m_PhysicalDevice.getSurfaceFormatsKHR(context.m_Surface));

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
            context.m_Surface,
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
        swapChainCreateInfo.presentMode = choose_swap_present_mode(context.m_PhysicalDevice.getSurfacePresentModesKHR(context.m_Surface));
        swapChainCreateInfo.clipped = true;
        swapChainCreateInfo.oldSwapchain = nullptr;

        m_SwapChain = vk::raii::SwapchainKHR(context.m_Device, swapChainCreateInfo);

        m_Images = m_SwapChain.getImages();
    }

    SwapChain SwapChain::create(Window &window, Context &context)
    {
        KAILUX_LOG_PARENT_CLR_CYAN("[SWAPCHAIN]")
        SwapChain swapChain;

        swapChain.createSwapChain(window, context);
        KAILUX_LOG_CHILD_CLR_CYAN("Swap chain created")

        return swapChain;
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
        for (auto presentMode: availablePresentModes)
            if (presentMode == vk::PresentModeKHR::eMailbox)
                return presentMode;

        return vk::PresentModeKHR::eFifo;
    }
}
