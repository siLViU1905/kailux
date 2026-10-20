# Kailux

A lightweight, modern C++ rendering engine.

## Kailux, Prism and Spectrum

The project is split in three parts:

- **kailux** is the engine itself: rendering (Vulkan), scenes, physics, scripting, and the application core (window, main loop, layer stack). It is built as a library.
- **prism** is the editor built on top of Kailux. It is a layer (`EditorLayer`) pushed onto a Kailux `Application`, providing the viewport, entity hierarchy, entity editor, asset browser, console and simulation view.
- **spectrum** is the runtime: it plays a saved scene full-window, without the editor. It is a layer (`RuntimeLayer`) pushed onto a Kailux `Application` started in runtime mode.

```
kailux - the engine library
prism  - the editor executable
spectrum  - the runtime executable
samples   - small games made with the scripting API
```

## Requirements
- Vulkan 1.3+
- conan
- cmake
- c++23 compatible compiler

## Building the Project

## Windows
**In the bat file the generator is set to "Visual Studio 18 2026". If you don't have it, use your Visual Studio version (eg: "Visual Studio 17 2022").**

Run the provided batch file with a build type:

```bat
scripts\windows_build.bat debug
scripts\windows_build.bat release
scripts\windows_build.bat debug clean
scripts\windows_build.bat debug samples
```

In the build folder you will find the sln file — open it with Visual Studio and build.

## Linux

Run the provided shell script with a build type:

```bash
scripts/linux_build.sh debug
scripts/linux_build.sh release
scripts/linux_build.sh debug clean
scripts/linux_build.sh debug samples
```

In the build folder you will find the executables: `build/[debug|release]/prism/prism` and `build/[debug|release]/spectrum/spectrum`.
Each executable gets its own copy of `shaders/` and `assets/` next to it, so run it from its own folder.


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

## Running a scene with Spectrum
Save a scene from Prism (`.klx`), then pass it to Spectrum:
```bash
cd build/debug/spectrum
./spectrum path/to/scene.klx

Spectrum renders the scene from the primary camera with the highest MSAA level the GPU supports(can be changed in `Context::GetMaxUsableSampleCount()`), the same way as the simulation view in Prism. The scene needs a camera; without one, Spectrum shows a black screen and the simulation does not start.
## Samples
`samples/` holds small games built on Kailux with the scripting API. They are not built by default; turn them on with the `KAILUX_BUILD_SAMPLES` CMake option.
With the build scripts, add `samples` to the arguments (it can be combined with `clean`):
```bash
scripts/linux_build.sh debug samples
```
```bat
scripts\windows_build.bat debug samples
```
The option is passed on every run, so running a script without `samples` turns them off again. With CMake directly (or in your IDE's CMake options), pass `-DKAILUX_BUILD_SAMPLES=ON`. To build a single sample, use its target, e.g. `cmake --build . --target sphere_shooter`.
Each sample gets `shaders/`, `assets/` and its own `scenes/` copied next to the executable, so run it from its own folder.
### Sphere Shooter
Loads `scenes/arena.klx` (or the scene passed as the first argument) and shoots dynamic spheres from the camera.

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
`force`, `fireRate`, `moveSpeed`, `sensitivity` and `maxBullets` are script parameters on the `Shooter` entity in the scene file and can be changed without recompiling.