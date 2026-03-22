#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "Context.h"
#include "Core.h"

namespace kailux
{
    class OneTimeCommand
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(OneTimeCommand)

        static void create_command_pool(const Context& context);
        static void destroy_command_pool();

        //already calls begin()
        static OneTimeCommand create(const Context& context);

        //already calls end()
        void submit(const Context& context) const;

        vk::CommandBuffer getCommandBuffer() const;

    private:
        void createBuffer(const Context& context);

        static vk::raii::CommandPool   s_CommandPool;

        vk::raii::CommandBuffer        m_CommandBuffer;
    };
}
