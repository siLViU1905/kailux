#pragma once
#include "audio/AudioRegistry.h"

namespace kailux
{
    struct AudioSourceComponent
    {
        std::string         clip;
        AudioSourceSettings settings;

        AudioSourceHandle source;
        std::string       loadedClip;
        bool              loadFailed{};
        bool              resumeOnStart{};

        glm::vec3 lastPosition{0.f};
        bool      hasLastPosition{};
    };
}