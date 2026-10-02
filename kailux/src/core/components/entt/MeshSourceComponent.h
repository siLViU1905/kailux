#pragma once
#include "core/Core.h"

namespace kailux
{
    struct MeshSourceComponent
    {
        std::string path;
        MeshType    type{MeshType::Unknown};
    };
}
