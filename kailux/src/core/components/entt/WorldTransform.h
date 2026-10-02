#pragma once

namespace kailux
{
    struct WorldTransform
    {
        glm::mat4 model{1.f};

        constexpr glm::vec3 GetPosition() const
        {
            return glm::vec3{model[3]};
        }
    };
}