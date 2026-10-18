#pragma once

#include "Input.h"
#include "Time.h"
#include "World.h"

namespace kailux
{
    struct ScriptContext
    {
        World world;
        Input input;
        Time  time;
    };
}