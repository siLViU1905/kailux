#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "core/Context.h"

namespace kailux
{
    class ImGuiBackend
    {
    public:
        ImGuiBackend();
        ImGuiBackend(const ImGuiBackend&) = delete;
        ImGuiBackend& operator=(const ImGuiBackend&) = delete;
        ImGuiBackend(ImGuiBackend&& other) noexcept;
        ImGuiBackend& operator=(ImGuiBackend&& other) noexcept;

        static ImGuiBackend create(const Context& context);

    private:
        void createDescriptorPool(const Context& context);

        vk::raii::DescriptorPool m_DescriptorPool;
    };
}
