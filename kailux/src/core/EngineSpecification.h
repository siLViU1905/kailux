#pragma once
#include <cstdint>

namespace kailux
{
    enum class RenderMode : uint8_t
    {
        Editor,
        Runtime
    };

    struct EngineSpecification
    {
        RenderMode renderMode{RenderMode::Editor};

        bool useImgui{};

        float physicsTickRate{1.f / 60.f};
    };
}