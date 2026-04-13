# Copilot Instructions

## Project Guidelines
- Ask before implementing things.
- Use non-inverted vertical mouse look for camera controls.
- For web builds, use an Emscripten-only path and avoid building Dawn unless it is specifically required for native desktop WebGPU support.
- Never update external dependencies' source or CMake files without explicit instructions to do so. Prefer alternatives in project-owned build configuration.
- Public methods and functions in project-owned headers should use Visual Studio XML documentation comments rather than plain comments.