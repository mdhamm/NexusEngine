# NexusEngine

`NexusEngine` is a personal C++ game engine project for learning, experimentation, and exploring engine architecture hands-on. It is designed around an ECS-first architecture with multiplatform support, with a major focus on web deployment through `WebGPU` while still supporting native desktop runtimes.

Part of the motivation for this project is the current gap in modern web support across major engines. `Unreal Engine` no longer supports `WebGL`, and `Unity` has had more limited support around web targets and `DOTS`, so this repository is also a space to experiment with what a web-focused, ECS-driven engine can look like.

The long-term direction for the engine is to provide a strong foundation for `MMO`-style games, including future networking support built around a server-authoritative model with client-side prediction and reconciliation.

At a high level, the project includes:
- `NexusEngine`: the core engine library
- `SampleGame`: a sample gameplay module built on top of the engine
- `GameRuntime`: the executable used to launch the sample game
- `Editor`: a desktop-only editor target

The engine currently uses:
- `Diligent Engine` for rendering
- `flecs` for ECS
- `SDL2` for windowing and input

The sample content sets up a scene with a fly camera, procedural cube terrain, and a few animated entities, which makes this repository a good starting point for engine experimentation and rendering work.

## How to run

### Desktop build

Requirements:
- Visual Studio with MSVC
- CMake 3.20 or newer
- Ninja

Configure and build:
- `cmake --preset x64-debug`
- `cmake --build out/build/x64-debug --target GameRuntime`

Run:
- `out/build/x64-debug/GameRuntime/GameRuntime.exe`

If you want the editor instead, build `Editor` on a desktop preset.

### Web build

Requirements:
- Emscripten SDK `4.0.0`
- CMake 3.20 or newer
- Ninja
- Node.js

For web with WebGPU, `emsdk 4.0.0` is required. Later versions break `Diligent Engine` in this project.

Configure and build:
- `cmake --preset web-debug`
- `cmake --build out/build/web-debug --target GameRuntime`

Serve the generated files from the repository root:
- `node tools/web-server/server.js out/build/web-debug/GameRuntime 8000`

Then open:
- `http://localhost:8000/`

For the default release web output, you can also use:
- `cd tools/web-server`
- `npm start`
