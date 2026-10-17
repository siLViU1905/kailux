#pragma once

namespace kailux
{
    struct PhysicsControlComponent
    {
        glm::vec3 velocity{0.f};

        glm::vec3 force{0.f};
        bool      applyForce{false};

        glm::vec3 impulse{0.f};
        bool      applyImpulse{false};
    };
}