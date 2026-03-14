#pragma once
#include "Context.h"

namespace kailux
{
    class SwapChain
    {
    public:
        SwapChain();
        SwapChain(const SwapChain&) = delete;
        SwapChain& operator=(const SwapChain&) = delete;
        SwapChain(SwapChain&& other) noexcept;
        SwapChain& operator=(SwapChain&& other) noexcept;

        static SwapChain create(Window& window, Context& context);

    private:
        static vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
        static vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR &capabilities, Window& window);
        static vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR> &availablePresentModes);

        void createSwapChain(Window& window, Context& context);

        vk::raii::SwapchainKHR           m_SwapChain;
        std::vector<vk::Image>           m_Images;
        std::vector<vk::raii::ImageView> m_ImageViews;
        vk::Format                       m_ImageFormat;
        vk::Extent2D                     m_Extent;
        vk::SurfaceFormatKHR             m_SurfaceFormat;
    };
}
