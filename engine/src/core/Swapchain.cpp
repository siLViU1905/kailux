#include "Swapchain.h"

#include "Log.h"

namespace kailux
{
    Swapchain::Swapchain() : mSwapchain(nullptr),
                             mColorImage({}),
                             mDepthImage({}),
                             mColorImageMemory({}),
                             mDepthImageMemory({}),
                             mColorImageView({}),
                             mDepthImageView({}),
                             mImageFormat(),
                             mDepthFormat(),
                             mExtent({}),
                             mSurfaceFormat({}),
                             mSemaphoreIndex(0)
    {
    }

    Swapchain::Swapchain(Swapchain &&other) noexcept : mSwapchain(std::move(other.mSwapchain)),
                                                       mImages(std::move(other.mImages)),
                                                       mColorImage(std::move(other.mColorImage)),
                                                       mDepthImage(std::move(other.mDepthImage)),
                                                       mColorImageMemory(std::move(other.mColorImageMemory)),
                                                       mDepthImageMemory(std::move(other.mDepthImageMemory)),
                                                       mImageViews(std::move(other.mImageViews)),
                                                       mColorImageView(std::move(other.mColorImageView)),
                                                       mDepthImageView(std::move(other.mDepthImageView)),
                                                       mImageFormat(other.mImageFormat),
                                                       mDepthFormat(other.mDepthFormat),
                                                       mExtent(other.mExtent),
                                                       mSurfaceFormat(other.mSurfaceFormat),
                                                       mAcquireSemaphores(std::move(other.mAcquireSemaphores)),
                                                       mPresentSemaphores(std::move(other.mPresentSemaphores)),
                                                       mSemaphoreIndex(0)
    {
    }

    Swapchain &Swapchain::operator=(Swapchain &&other) noexcept
    {
        if (this != &other)
        {
            mSwapchain = std::move(other.mSwapchain);
            mImages = std::move(other.mImages);
            mColorImage = std::move(other.mColorImage);
            mDepthImage = std::move(other.mDepthImage);
            mColorImageMemory = std::move(other.mColorImageMemory);
            mDepthImageMemory = std::move(other.mDepthImageMemory);
            mImageViews = std::move(other.mImageViews);
            mColorImageView = std::move(other.mColorImageView);
            mDepthImageView = std::move(other.mDepthImageView);
            mImageFormat = other.mImageFormat;
            mDepthFormat = other.mDepthFormat;
            mExtent = other.mExtent;
            mSurfaceFormat = other.mSurfaceFormat;
            mAcquireSemaphores = std::move(other.mAcquireSemaphores);
            mPresentSemaphores = std::move(other.mPresentSemaphores);
            mSemaphoreIndex = other.mSemaphoreIndex;
        }
        return *this;
    }

    void Swapchain::createSwapchain(const Window &window, const Context &context)
    {
        auto surfaceCapabilities = context.getPhysicalDevice().getSurfaceCapabilitiesKHR(context.getSurface());

        mSurfaceFormat = choose_swap_surface_format(
            context.getPhysicalDevice().getSurfaceFormatsKHR(context.getSurface()));

        mExtent = choose_swap_extent(surfaceCapabilities, window);

        mImageFormat = mSurfaceFormat.format;

        uint32_t requestedImageCount = surfaceCapabilities.minImageCount + 2;

        if (surfaceCapabilities.maxImageCount > 0 && requestedImageCount > surfaceCapabilities.maxImageCount)
            requestedImageCount = surfaceCapabilities.maxImageCount;

        vk::SwapchainCreateInfoKHR swapChainCreateInfo{
            vk::SwapchainCreateFlagsKHR(),
            context.getSurface(),
            requestedImageCount,
            mSurfaceFormat.format,
            mSurfaceFormat.colorSpace,
            mExtent,
            1,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::SharingMode::eExclusive
        };

        swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
        swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        swapChainCreateInfo.presentMode = choose_swap_present_mode(
            context.getPhysicalDevice().getSurfacePresentModesKHR(context.getSurface()));
        swapChainCreateInfo.clipped = true;
        swapChainCreateInfo.oldSwapchain = *mSwapchain;

        mSwapchain = vk::raii::SwapchainKHR(context.mDevice, swapChainCreateInfo);

        mImages = mSwapchain.getImages();
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
        window.getFramebufferSize(width, height);

        if (static_cast<uint32_t>(width) == mExtent.width &&
            static_cast<uint32_t>(height) == mExtent.height)
            return;

        while (width == 0 || height == 0)
        {
            window.getFramebufferSize(width, height);
            window.waitForEvents();
        }

        context.getDevice().waitIdle();

        mImageViews.clear();
        mImages.clear();
        mAcquireSemaphores.clear();
        mPresentSemaphores.clear();
        mSemaphoreIndex = 0;

        createSwapchain(window, context);
        createImageViews(context);
        createColorResources(context, sampleCount);
        createDepthResources(context, sampleCount);
        createSyncObjects(context);
        KAILUX_LOG_INFO("[Swapchain]",
                        std::format("Recreated with extent: x:{}, y:{}", mExtent.width, mExtent.height))
    }

    vk::Format Swapchain::getFormat() const
    {
        return mImageFormat;
    }

    vk::Format Swapchain::getDepthFormat() const
    {
        return mDepthFormat;
    }

    vk::Extent2D Swapchain::getExtent() const
    {
        return mExtent;
    }

    vk::Image Swapchain::getImage(uint32_t index) const
    {
        return mImages[index];
    }

    vk::Image Swapchain::getColorImage() const
    {
        return *mColorImage;
    }

    vk::Image Swapchain::getDepthImage() const
    {
        return *mDepthImage;
    }

    vk::ImageView Swapchain::getImageView(uint32_t index) const
    {
        return *mImageViews[index];
    }

    vk::ImageView Swapchain::getColorImageView() const
    {
        return *mColorImageView;
    }

    vk::ImageView Swapchain::getDepthImageView() const
    {
        return *mDepthImageView;
    }

    uint32_t Swapchain::getImageCount() const
    {
        return static_cast<uint32_t>(mImages.size());
    }

    std::optional<Swapchain::AcquireResult> Swapchain::acquire()
    {
        auto semaphore = *mAcquireSemaphores[mSemaphoreIndex];
        try
        {
            auto [result, imageIndex] = mSwapchain.acquireNextImage(std::numeric_limits<uint64_t>::max(), semaphore, nullptr);

            mSemaphoreIndex = (mSemaphoreIndex + 1) % mAcquireSemaphores.size();

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
        return *mPresentSemaphores[index];
    }

    bool Swapchain::present(const Context &context, uint32_t imageIndex, vk::Semaphore renderFinishedSemaphore) const
    {
        vk::PresentInfoKHR presentInfo{
            renderFinishedSemaphore,
            *mSwapchain,
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
        static constexpr std::array preferredPresentModes = {
            vk::PresentModeKHR::eMailbox
            // vk::PresentModeKHR::eFifoLatestReady
        };

        for (auto presentMode : availablePresentModes)
            if (std::ranges::contains(preferredPresentModes, presentMode))
                return presentMode;

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
        mImageViews.clear();
        mImageViews.reserve(mImages.size());

        for (auto image: mImages)
        {
            vk::ImageViewCreateInfo viewInfo{
                {},
                image,
                vk::ImageViewType::e2D,
                mImageFormat,
                {},
                vk::ImageSubresourceRange{
                    vk::ImageAspectFlagBits::eColor,
                    0, 1,
                    0, 1
                }
            };

            mImageViews.emplace_back(context.mDevice, viewInfo);
        }
    }

    void Swapchain::createColorResources(const Context &context, vk::SampleCountFlagBits sampleCount)
    {
        if (sampleCount == vk::SampleCountFlagBits::e1)
            return;

        vk::ImageCreateInfo imageInfo(
            {},
            vk::ImageType::e2D,
            mImageFormat,
            vk::Extent3D(mExtent.width, mExtent.height, 1),
            1,
            1,
            sampleCount,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment
        );

        mColorImage = vk::raii::Image(context.mDevice, imageInfo);

        auto memReqs = mColorImage.getMemoryRequirements();
        mColorImageMemory = vk::raii::DeviceMemory(context.mDevice,
                                                    {
                                                        memReqs.size,
                                                        context.findMemoryType(
                                                            memReqs.memoryTypeBits,
                                                            vk::MemoryPropertyFlagBits::eDeviceLocal)
                                                    });
        mColorImage.bindMemory(*mColorImageMemory, 0);

        mColorImageView = vk::raii::ImageView(context.mDevice,
                                               {
                                                   {},
                                                   *mColorImage,
                                                   vk::ImageViewType::e2D,
                                                   mImageFormat,
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
        mDepthFormat = find_depth_format(context);

        vk::ImageCreateInfo imageInfo{
            {},
            vk::ImageType::e2D,
            mDepthFormat,
            vk::Extent3D{mExtent.width, mExtent.height, 1},
            1, 1,
            sampleCount,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment
        };

        mDepthImage = vk::raii::Image(context.mDevice, imageInfo);

        vk::MemoryRequirements memReqs = mDepthImage.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{
            memReqs.size,
            context.findMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
        };
        mDepthImageMemory = vk::raii::DeviceMemory(context.mDevice, allocInfo);
        mDepthImage.bindMemory(*mDepthImageMemory, 0);

        vk::ImageViewCreateInfo viewInfo{
            {},
            *mDepthImage,
            vk::ImageViewType::e2D,
            mDepthFormat,
            {},
            vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}
        };

        mDepthImageView = vk::raii::ImageView(context.mDevice, viewInfo);
    }

    void Swapchain::createSyncObjects(const Context &context)
    {
        mAcquireSemaphores.reserve(mImages.size());
        mPresentSemaphores.reserve(mImages.size());

        for (size_t i = 0; i < mImages.size(); i++)
        {
            mAcquireSemaphores.emplace_back(context.mDevice, vk::SemaphoreCreateInfo{});
            mPresentSemaphores.emplace_back(context.mDevice, vk::SemaphoreCreateInfo{});
        }
    }
}
