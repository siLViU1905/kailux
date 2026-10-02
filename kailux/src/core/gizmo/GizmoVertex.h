#pragma once
#include <glm/vec3.hpp>

namespace kailux
{
    struct GizmoVertex
    {
        glm::vec3 position{};

        static constexpr vk::VertexInputBindingDescription get_binding_description()
        {
            return {0, sizeof(GizmoVertex), vk::VertexInputRate::eVertex};
        }

        static constexpr std::array<vk::VertexInputAttributeDescription, 1> get_attribute_description()
        {
            return {
                vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(GizmoVertex, position))
            };
        }
    };
}