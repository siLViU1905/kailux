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

        void createSwapChain(Window& window, const Context& context);
        void createImageViews(const Context& context);
        void createSyncObjects(const Context& context);

        vk::raii::SwapchainKHR           m_SwapChain;
        std::vector<vk::Image>           m_Images;
        std::vector<vk::raii::ImageView> m_ImageViews;
        vk::Format                       m_ImageFormat;
        vk::Extent2D                     m_Extent;
        vk::SurfaceFormatKHR             m_SurfaceFormat;
        std::vector<vk::raii::Semaphore> m_AcquireSemaphores;
        std::vector<vk::raii::Semaphore> m_PresentSemaphores;
        uint32_t                         m_SemaphoreIndex;
    };
}
