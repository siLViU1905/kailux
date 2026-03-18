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

        static SwapChain create(Window& window, const Context& context);

        void recreate(Window& window, const Context& context);

        vk::Format              getFormat() const;
        vk::Format              getDepthFormat() const;
        vk::Extent2D            getExtent() const;
        vk::Image               getImage(uint32_t index) const;
        vk::ImageView           getImageView(uint32_t index) const;
        uint32_t                getImageCount() const;

        struct AcquireResult
        {
            uint32_t      imageIndex;
            vk::Semaphore imageAvailableSemaphore;
        };
        std::optional<AcquireResult> acquire();
        vk::Semaphore getPresentSemaphore(uint32_t index) const;

        bool present(const Context& context, uint32_t imageIndex, vk::Semaphore renderFinishedSemaphore) const;

    private:
        static vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
        static vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR &capabilities, Window& window);
        static vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
        static vk::Format find_depth_format(const Context& context);

        void createSwapChain(Window& window, const Context& context);
        void createImageViews(const Context& context);
        void createDepthResources(const Context& context);
        void createSyncObjects(const Context& context);

        vk::raii::SwapchainKHR           m_SwapChain;
        std::vector<vk::Image>           m_Images;
        vk::raii::Image                  m_DepthImage;
        vk::raii::DeviceMemory           m_DepthImageMemory;
        std::vector<vk::raii::ImageView> m_ImageViews;
        vk::raii::ImageView              m_DepthImageView;
        vk::Format                       m_ImageFormat;
        vk::Format                       m_DepthFormat;
        vk::Extent2D                     m_Extent;
        vk::SurfaceFormatKHR             m_SurfaceFormat;
        std::vector<vk::raii::Semaphore> m_AcquireSemaphores;
        std::vector<vk::raii::Semaphore> m_PresentSemaphores;
        uint32_t                         m_SemaphoreIndex;
    };
}
