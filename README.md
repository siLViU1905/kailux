# Kailux

A lightweight, modern C++ rendering engine built on Vulkan.

## Features

**Rendering**
- Vulkan 1.3+ renderer with dynamic rendering, bindless textures (descriptor indexing) and MSAA
- PBR materials with image based lighting (irradiance + prefiltered cubemaps, BRDF LUT) and a skybox
- Point and directional lights, cascaded shadow maps (4 cascades, configurable max shadow distance) and up to 4 point light shadows
- GPU driven drawing: compute frustum culling, command compaction and indirect instanced draws, scaled for up to 10 000 meshes
- Automatic mesh LODs and vertex cache / overdraw optimization (meshoptimizer)
- Compute particle system
- GPU entity picking (compute) and selection outline
- Runtime GLSL compilation with shaderc

**Scene and editor**
- ECS scene (EnTT) with entity hierarchy, primitives (plane, cube, sphere) and models loaded with Assimp
- Scenes saved as `.klx` (JSON); asset paths are stored relative to the scene, and saving copies external assets into `<scene>_assets/` next to the scene so the scene folder can be moved
- Separate material panel for editing PBR parameters and textures
- The editor talks to the engine through a scene command queue
- Asynchronous asset loading (multiple meshes per poll)

**Physics**
- Jolt Physics: static, kinematic and dynamic bodies, collision and trigger callbacks, raycasts, physics interpolation

**Audio**
- 3D audio with OpenAL Soft: audio sources with gain, pitch, looping, play on start, spatialization and distance attenuation
- WAV, MP3 and FLAC decoding (dr_libs)

**Scripting**
- Native C++ scripts with a small API layer between the scripts and the engine (`scripting/ScriptAPI.h`)
- Script parameters stored in the scene file and reloaded live when the scene file changes on disk

**Profiling**
- Optional CPU / GPU profiling scopes (`KAILUX_ENABLE_PROFILING`) and a benchmark sample that writes the results to a file

## Kailux, Prism and Spectrum

The project is split in three parts:

- **kailux** is the engine itself: rendering (Vulkan), scenes, physics, audio, particles, scripting, and the application core (window, main loop, layer stack). It is built as a library.
- **prism** is the editor built on top of Kailux. It is a layer (`EditorLayer`) pushed onto a Kailux `Application`, providing the viewport, entity hierarchy, entity editor, material editor, asset browser, console and simulation view.
- **spectrum** is the runtime: it plays a saved scene full-window, without the editor. It is a layer (`RuntimeLayer`) pushed onto a Kailux `Application` started in runtime mode.

```
kailux    - the engine library
prism     - the editor executable
spectrum  - the runtime executable
samples   - small games made with the scripting API
```

## Requirements
- A GPU with Vulkan 1.3+ support and the Vulkan SDK (1.4)
- conan 2
- cmake 3.31+
- Ninja
- c++23 compatible compiler (MSVC, GCC or Clang)

Optional: `ccache` is used automatically when found, and on Linux `mold` or `lld` is used as the linker when found.

## Building the Project

Both build scripts take the same arguments:

| Argument | Effect |
|---|---|
| `debug` / `release` | Build type (required) |
| `clean` | Remove `build/` before building |
| `samples` | Also build the sample games (`KAILUX_BUILD_SAMPLES`) |
| `profiling` | Compile the CPU/GPU profiling scopes and build the benchmark sample (`KAILUX_ENABLE_PROFILING`) |
| `unity` | Unity build, compiles the sources in batches for faster full builds (`KAILUX_UNITY_BUILD`) |

The options are passed on every run, so running a script without an argument turns that option off again.

The scripts export the local `imgui` and `imguizmo` conan recipes from `scripts/`, run `conan install` (only the first time for a build type) and build with Ninja.

## Windows

Run the provided batch file from a terminal:

```bat
scripts\windows_build.bat debug
scripts\windows_build.bat release
scripts\windows_build.bat debug clean
scripts\windows_build.bat debug samples
scripts\windows_build.bat release samples profiling
scripts\windows_build.bat release unity
```

The script sets up the MSVC environment from conan (`conanbuild.bat`) and builds with Ninja. The executables are in `build\[debug|release]\prism\prism.exe` and `build\[debug|release]\spectrum\spectrum.exe`.

## Linux

Run the provided shell script:

```bash
scripts/linux_build.sh debug
scripts/linux_build.sh release
scripts/linux_build.sh debug clean
scripts/linux_build.sh debug samples
scripts/linux_build.sh release samples profiling
scripts/linux_build.sh release unity
```

In the build folder you will find the executables: `build/[debug|release]/prism/prism` and `build/[debug|release]/spectrum/spectrum`.

Each executable gets its own copy of `shaders/`, `assets/`, `licenses/` and the OpenAL Soft shared library next to it, so run it from its own folder.

## Prism viewport controls

The scene viewport uses a Blender-style orbit camera that rotates around a pivot in front of it.

| Input | Action |
|---|---|
| Middle mouse drag | Orbit around the pivot |
| Shift + middle mouse drag | Pan |
| Mouse wheel | Zoom in / out |
| Click an axis on the view gizmo (top right) | View the scene along that axis |
| Drag the view gizmo | Orbit around the pivot |
| Left click | Select an entity |
| Right mouse button | Toggle mouse look (fly mode) |
| WASD, Space, Ctrl | Move the scene camera (while mouse look is on) |

The simulation view keeps the runtime controls: middle mouse button toggles mouse look for the primary camera.

## Saving scenes

Scenes are saved from Prism as `.klx` files. Asset paths (meshes, textures, audio clips) are stored relative to the scene. Assets that are outside the scene folder are copied into `<scene name>_assets/` next to the scene when saving, so the scene and its assets can be moved or shipped together.

While a scene is open, the scene file is watched: editing the script parameters in the `.klx` file reloads them in real time, without restarting the simulation (scripts get `OnParamsChanged()`).

## Running a scene with Spectrum
Save a scene from Prism (`.klx`), then pass it to Spectrum:
```bash
cd build/debug/spectrum
./spectrum path/to/scene.klx
```

Spectrum renders the scene from the primary camera with the highest MSAA level the GPU supports (can be changed in `Context::GetMaxUsableSampleCount()`), the same way as the simulation view in Prism. The scene needs a camera; without one, Spectrum shows a black screen and the simulation does not start.

## Scripting

Scripts are C++ classes deriving from `kailux::Script`, registered with `KAILUX_REGISTER_SCRIPT` and attached to entities in the scene. Include `scripting/ScriptAPI.h` to get the whole API.

```cpp
#include "scripting/ScriptAPI.h"

class Spinner final : public kailux::Script
{
protected:
    void OnUpdate(float deltaTime) override
    {
        const auto speed{GetParams().Get<float>("speed", 90.f)};
        // ...
    }
};

KAILUX_REGISTER_SCRIPT(Spinner);
```

| Part | What it gives |
|---|---|
| `Script` | Callbacks: `OnCreate`, `OnUpdate`, `OnFixedUpdate`, `OnDestroy`, `OnParamsChanged`, `OnCollisionEnter/Exit`, `OnTriggerEnter/Exit` |
| `Entity` | Transform, components, material (`MaterialRef`), rigid body (`RigidBody`), audio source and particle emitter |
| `World` | Find and spawn entities (empty, primitives, meshes), raycasts, main camera |
| `Input` | Keyboard and mouse state, mouse delta, cursor mode |
| `Time` | Time and delta time |
| `AudioSource` | Play / pause / stop, clip, gain and the other source settings |
| `ParticleEmitter` | Bursts, emitting on / off, rate, emitter settings |
| `ScriptParams` | Parameters set per entity in the scene file |

## Samples
`samples/` holds small games built on Kailux with the scripting API. They are not built by default; turn them on with the `KAILUX_BUILD_SAMPLES` CMake option.
With the build scripts, add `samples` to the arguments (it can be combined with the other arguments):
```bash
scripts/linux_build.sh debug samples
```
```bat
scripts\windows_build.bat debug samples
```
With CMake directly (or in your IDE's CMake options), pass `-DKAILUX_BUILD_SAMPLES=ON`. To build a single sample, use its target, e.g. `cmake --build . --target sphere_shooter`.
Each sample gets `shaders/`, `assets/` and its own `scenes/` (and `sounds/`) copied next to the executable, so run it from its own folder.

### Sphere Shooter
Loads `scenes/arena.klx` (or the scene passed as the first argument) and shoots dynamic spheres from the camera, with a 3D sound on impact.

```bash
cd build/debug/samples/sphere_shooter
./sphere_shooter
```
| Input | Action |
|---|---|
| Mouse | Look around |
| WASD | Move on the ground plane |
| Space, Left Ctrl | Move up / down |
| Left Shift | Move twice as fast |
| Left mouse button (hold) | Shoot |
| P | Pause / resume |

`force`, `fireRate`, `moveSpeed`, `sensitivity` and `maxBullets` are script parameters on the `Shooter` entity in the scene file and can be changed without recompiling, even while the game is running.

### Benchmark
`sphere_shooter_benchmark` runs the Sphere Shooter scene and records the CPU and GPU timings of the engine (per pass, plus draw call counts). It is built only with both `samples` and `profiling`:

```bash
scripts/linux_build.sh release samples profiling
cd build/release/samples/benchmark
./sphere_shooter_benchmark [scene.klx] [output.txt]
```

After a 5 second warm-up it writes a sample every 0.5 seconds to `benchmarks/benchmark_<date>_<time>.txt` (or the given output file).

## Dependencies

All dependencies are pulled with conan (`conanfile.py`), except the Vulkan SDK which must be installed on the system. `imgui` and `imguizmo` use the local recipes from `scripts/` (Dear ImGui docking branch and its GLFW/Vulkan backends with local patches).

| Dependency | Version | Used for | License |
|---|---|---|---|
| [Vulkan SDK](https://vulkan.lunarg.com/) | 1.4 | Graphics API (`vulkan.hpp`, `vulkan_raii.hpp`) | Apache-2.0 / MIT |
| [GLFW](https://www.glfw.org/) | 3.4 | Window, input and Vulkan surface | Zlib |
| [Dear ImGui](https://github.com/ocornut/imgui) (docking) | 1.92.9b | Editor UI, runtime overlays (benchmark) | MIT |
| imgui_backends (local recipe) | 1.92.9b | ImGui GLFW / Vulkan backends and `imgui_stdlib` | MIT |
| [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) | cci.20261010 | Transform gizmos and view gizmo in the viewport | MIT |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | Math (vectors, matrices, quaternions) | MIT |
| [EnTT](https://github.com/skypjack/entt) | 3.16.0 | Entity component system for scenes | MIT |
| [magic_enum](https://github.com/Neargye/magic_enum) | 0.9.7 | Enum to string (logs, UI, profiler sections) | MIT |
| [stb](https://github.com/nothings/stb) | cci.20230920 | Image loading (`stb_image`) for textures, cubemaps and IBL | MIT / Public domain |
| [Assimp](https://github.com/assimp/assimp) | 6.0.2 | 3D model import (meshes and material textures) | BSD-3-Clause |
| [portable-file-dialogs](https://github.com/samhocevar/portable-file-dialogs) | 0.1.0 | Native open / save file dialogs | WTFPL |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.12.0 | Scene files (`.klx`) and script parameters | MIT |
| [Jolt Physics](https://github.com/jrouwe/JoltPhysics) | 5.2.0 | Rigid body physics, collisions, triggers, raycasts | MIT |
| [shaderc](https://github.com/google/shaderc) | 2025.3 | Runtime GLSL to SPIR-V compilation | Apache-2.0 |
| [oneTBB](https://github.com/uxlfoundation/oneTBB) | 2023.1.0 | Parallel algorithms (`std::execution` backend for GCC) | Apache-2.0 |
| [meshoptimizer](https://github.com/zeux/meshoptimizer) | 1.0 | Mesh LOD generation, vertex cache and overdraw optimization | MIT |
| [OpenAL Soft](https://github.com/kcat/openal-soft) | 1.24.3 | 3D audio (linked as a shared library) | LGPL-2.0-or-later |
| [dr_libs](https://github.com/mackron/dr_libs) | cci.20230529 | WAV / MP3 / FLAC decoding | Public domain / MIT-0 |
| [JetBrains Mono](https://www.jetbrains.com/lp/mono/) (font, `assets/fonts`) | - | Editor font | OFL-1.1 |

## License

Kailux is released under the MIT License, see [LICENSE](LICENSE). OpenAL Soft is LGPL and is linked dynamically; its license is in `licenses/openal-soft/` and is copied next to every executable.
