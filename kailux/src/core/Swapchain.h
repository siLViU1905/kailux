#pragma once
#include "Context.h"

namespace kailux
{
    class Swapchain
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Swapchain)

        static Swapchain create(Window& window, const Context& context, vk::SampleCountFlagBits sampleCount);

        void Recreate(const Window &window, const Context& context, vk::SampleCountFlagBits sampleCount);

        vk::Format              GetFormat() const;
        vk::Format              GetDepthFormat() const;
        vk::Extent2D            GetExtent() const;
        vk::Image               GetImage(uint32_t index) const;
        vk::Image               GetColorImage() const;
        vk::Image               GetDepthImage() const;
        vk::ImageView           GetImageView(uint32_t index) const;
        vk::ImageView           GetColorImageView() const;
        vk::ImageView           GetDepthImageView() const;
        uint32_t                GetImageCount() const;

        struct AcquireResult
        {
            uint32_t      imageIndex;
            vk::Semaphore imageAvailableSemaphore;
        };
        std::optional<AcquireResult> Acquire();
        vk::Semaphore GetPresentSemaphore(uint32_t index) const;

        bool Present(const Context& context, uint32_t imageIndex, vk::Semaphore renderFinishedSemaphore) const;

    private:
        static vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
        static vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR &capabilities, const Window &window);
        static vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
        static vk::Format find_depth_format(const Context& context);

        void CreateSwapchain(const Window& window, const Context& context);
        void CreateImageViews(const Context& context);
        void CreateColorResources(const Context &context, vk::SampleCountFlagBits sampleCount);
        void CreateDepthResources(const Context& context, vk::SampleCountFlagBits sampleCount);
        void CreateSyncObjects(const Context& context);

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
