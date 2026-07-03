#pragma once
#include "Context.h"

namespace kailux
{
    class Swapchain
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Swapchain)

        static Swapchain create(Window& window, const Context& context, vk::SampleCountFlagBits sampleCount);

        void recreate(const Window &window, const Context& context, vk::SampleCountFlagBits sampleCount);

        vk::Format              getFormat() const;
        vk::Format              getDepthFormat() const;
        vk::Extent2D            getExtent() const;
        vk::Image               getImage(uint32_t index) const;
        vk::Image               getColorImage() const;
        vk::Image               getDepthImage() const;
        vk::ImageView           getImageView(uint32_t index) const;
        vk::ImageView           getColorImageView() const;
        vk::ImageView           getDepthImageView() const;
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
        static vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR &capabilities, const Window &window);
        static vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
        static vk::Format find_depth_format(const Context& context);

        void createSwapchain(const Window& window, const Context& context);
        void createImageViews(const Context& context);
        void createColorResources(const Context &context, vk::SampleCountFlagBits sampleCount);
        void createDepthResources(const Context& context, vk::SampleCountFlagBits sampleCount);
        void createSyncObjects(const Context& context);

        vk::raii::SwapchainKHR           mSwapchain;
        std::vector<vk::Image>           mImages;
        vk::raii::Image                  mColorImage;
        vk::raii::Image                  mDepthImage;
        vk::raii::DeviceMemory           mColorImageMemory;
        vk::raii::DeviceMemory           mDepthImageMemory;
        std::vector<vk::raii::ImageView> mImageViews;
        vk::raii::ImageView              mColorImageView;
        vk::raii::ImageView              mDepthImageView;
        vk::Format                       mImageFormat;
        vk::Format                       mDepthFormat;
        vk::Extent2D                     mExtent;
        vk::SurfaceFormatKHR             mSurfaceFormat;
        std::vector<vk::raii::Semaphore> mAcquireSemaphores;
        std::vector<vk::raii::Semaphore> mPresentSemaphores;
        uint32_t                         mSemaphoreIndex;
    };
}
