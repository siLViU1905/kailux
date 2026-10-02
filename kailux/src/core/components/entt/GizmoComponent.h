#pragma once
#include "../../gizmo/GizmoRegistry.h"

namespace kailux
{
    struct GizmoComponent
    {
        GizmoHandle handle;
        float       scale{0.5f};
        glm::vec4   color{};
    };
}