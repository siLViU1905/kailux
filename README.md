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
```

In the build folder you will find the sln file — open it with Visual Studio and build.

## Linux

Run the provided shell script with a build type:

```bash
scripts/linux_build.sh debug
scripts/linux_build.sh release
scripts/linux_build.sh debug clean
```

In the build folder you will find the executables: `build/[debug|release]/prism/prism` and `build/[debug|release]/spectrum/spectrum`.
Each executable gets its own copy of `shaders/` and `assets/` next to it, so run it from its own folder.
## Running a scene with Spectrum
Save a scene from Prism (`.klx`), then pass it to Spectrum:
```bash
cd build/debug/spectrum
./spectrum path/to/scene.klx
```
| Input | Action |
|---|---|
| Middle mouse button | Toggle mouse look |
| Escape | Release the mouse |
| WASD, Space, Ctrl | Move the primary camera (while mouse look is on) |
| Tab | Switch to the next camera |
| P | Pause / resume the simulation |
Spectrum renders the scene from the primary camera with the highest MSAA level the GPU supports(can be changed in `Context::GetMaxUsableSampleCount()`), the same way as the simulation view in Prism. The scene needs a camera; without one, Spectrum shows a black screen and the simulation does not start.