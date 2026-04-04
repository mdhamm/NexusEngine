# Copilot Instructions

## Project Guidelines
- Use non-inverted vertical mouse look for camera controls.
- For transforms, prefer setter/getter-based local/world synchronization with lazy stale computation; propagation should resolve hierarchy-wide staleness rather than being the only way world state updates.
- For web builds, use an Emscripten-only path and avoid building Dawn unless it is specifically required for native desktop WebGPU support.
- This repo should vendor flecs under `extern/flecs` and not use `FetchContent` for flecs in `CMakeLists.txt`.