cmake_minimum_required(VERSION 3.20)
project(NexusRoot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------- Platform mode ----------------
option(BUILD_WEB "Build for Web (Emscripten)" OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(BUILD_WEB ON)
endif()

# ---------------- Diligent (extern) ----------------
# Keep only the backends we use: DX12 (desktop) + WebGPU (web)
set(DILIGENT_NO_OPENGL       ON  CACHE BOOL "" FORCE)
set(DILIGENT_NO_VULKAN       ON  CACHE BOOL "" FORCE)
set(DILIGENT_NO_DIRECT3D11   ON  CACHE BOOL "" FORCE)
set(DILIGENT_NO_WEBGPU       OFF CACHE BOOL "" FORCE)   # enable WebGPU support
if(NOT BUILD_WEB)
  set(DILIGENT_NO_DIRECT3D12 OFF CACHE BOOL "" FORCE)   # enable D3D12 on desktop
endif()
set(DILIGENT_BUILD_SAMPLES   OFF CACHE BOOL "" FORCE)
set(DILIGENT_BUILD_TESTS     OFF CACHE BOOL "" FORCE)

# Bring in Diligent sources (expects extern/DiligentEngine in your tree)
add_subdirectory(extern/DiligentEngine)

# ---------------- Dependencies via FetchContent ----------------
include(FetchContent)

# ---- Flecs (ECS) ----
# Flecs config (static lib, no tests/docs/tools)
set(FLECS_STATIC   ON  CACHE BOOL "" FORCE)
set(FLECS_SHARED   OFF CACHE BOOL "" FORCE)
set(FLECS_PIC      ON  CACHE BOOL "" FORCE)
set(FLECS_TESTS    OFF CACHE BOOL "" FORCE)
set(FLECS_DOCS     OFF CACHE BOOL "" FORCE)
set(FLECS_SANITIZE OFF CACHE BOOL "" FORCE)
set(FLECS_META     ON  CACHE BOOL "" FORCE)

FetchContent_Declare(
  flecs
  GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
  GIT_TAG v3.2.0
)
FetchContent_MakeAvailable(flecs) # provides flecs::flecs

# ---- ImGui (docking) ----
add_subdirectory(extern/imgui) 

# ---------------- SDL2 (shared iface for apps) ----------------
if(NOT BUILD_WEB)
  # Prefer static SDL on desktop (optional)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON  CACHE BOOL "" FORCE)

  FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG release-2.30.8
    FIND_PACKAGE_ARGS
  )
  FetchContent_MakeAvailable(SDL2)      # provides SDL2::SDL2 (+ SDL2::SDL2main)

  add_library(NEXUS_SDL INTERFACE)
  target_link_libraries(NEXUS_SDL INTERFACE SDL2::SDL2main SDL2::SDL2)
else()
  # Emscripten SDL2 port
  add_library(NEXUS_SDL INTERFACE)
  target_link_options(NEXUS_SDL INTERFACE "-sUSE_SDL=2")
endif()

# ---------------- Diligent alias interface targets ----------------
# (So downstream targets can just link DILIGENT_D3D12 / DILIGENT_WEBGPU)

# D3D12 alias
set(DILIGENT_D3D12_TARGET "")
foreach(t
  Diligent-GraphicsEngineD3D12
  Diligent-GraphicsEngineD3D12-static
  Diligent-GraphicsEngineD3D12-shared
)
  if(TARGET ${t})
    set(DILIGENT_D3D12_TARGET ${t})
    break()
  endif()
endforeach()

add_library(DILIGENT_D3D12 INTERFACE)
if(DILIGENT_D3D12_TARGET)
  target_link_libraries(DILIGENT_D3D12 INTERFACE ${DILIGENT_D3D12_TARGET})
endif()

# WebGPU alias
set(DILIGENT_WEBGPU_TARGET "")
foreach(t
  Diligent-GraphicsEngineWebGPU
  Diligent-GraphicsEngineWebGPU-static
  Diligent-GraphicsEngineWebGPU-shared
)
  if(TARGET ${t})
    set(DILIGENT_WEBGPU_TARGET ${t})
    break()
  endif()
endforeach()

add_library(DILIGENT_WEBGPU INTERFACE)
if(DILIGENT_WEBGPU_TARGET)
  target_link_libraries(DILIGENT_WEBGPU INTERFACE ${DILIGENT_WEBGPU_TARGET})
endif()

# ---------------- Subprojects ----------------
# ImGui/Flecs/SDL/Diligent are all available BEFORE NexusEngine,
# so the engine can include <imgui.h> and link its backends.
add_subdirectory(NexusEngine)   # STATIC lib (owns ImGui + bridge)
add_subdirectory(SampleGame)    # STATIC lib
add_subdirectory(GameRuntime)   # EXE
add_subdirectory(Editor)        # EXE
