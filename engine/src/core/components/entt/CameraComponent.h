#pragma once
#include "core/Camera.h"

namespace kailux
{
    struct CameraComponent
    {
        Camera camera;
        bool   isPrimary = true;
    };
}
