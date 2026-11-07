#pragma once
#include "particles/ParticleTypes.h"

namespace kailux 
{
    struct ParticleEmitterComponent
    {
        ParticleEmitterSettings settings;

        uint32_t pendingBurst{};      
        float    spawnAccumulator{};  
        float    lastSpawnTime{-1.f}; 
    };
}