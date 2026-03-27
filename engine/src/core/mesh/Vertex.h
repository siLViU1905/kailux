#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace kailux
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec4 tangent;

        static constexpr vk::VertexInputBindingDescription get_binding_description()
        {
            return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
        }

        static constexpr std::array<vk::VertexInputAttributeDescription, 4> get_attribute_description()
        {
            return {
                vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)),
                vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)),
                vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)),
                vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, tangent))
            };
        }
    };

    static_assert(sizeof(Vertex) % alignof(Vertex) == 0,
        "Vertex size must be a multiple of its alignment");
}