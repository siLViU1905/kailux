#pragma once
#include "Core.h"

namespace kailux
{
    struct MeshSourceComponent
    {
        std::string path;
        MeshType    type{MeshType::Unknown};
    };
}
