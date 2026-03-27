cmake_minimum_required(VERSION 3.20)
project(NexusEngine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(NexusEngine STATIC
  src/NexusEngine.cpp
  "src/common/GraphicsRenderer.cpp"
  src/Scene.cpp
  src/NexusEngine.h
  "src/common/GraphicsRenderer.h"
  src/Scene.h
  
  src/components/CameraComponent.h "src/components/MeshComponent.h" "src/components/RenderTextureComponent.h")

target_include_directories(NexusEngine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/common
  ${CMAKE_SOURCE_DIR}/extern/DiligentEngine
  ${CMAKE_SOURCE_DIR}/extern/imgui
)

# Diligent core
target_link_libraries(NexusEngine
  PUBLIC
    Diligent-Common
    Diligent-GraphicsEngine
    Diligent-TargetPlatform
    NEXUS_SDL
)

# Flecs
if(TARGET flecs::flecs)
  target_link_libraries(NexusEngine PUBLIC flecs::flecs)
elseif(TARGET flecs)
  target_link_libraries(NexusEngine PUBLIC flecs)
elseif(TARGET flecs_static)
  target_link_libraries(NexusEngine PUBLIC flecs_static)
  if(DEFINED flecs_SOURCE_DIR)
    target_include_directories(NexusEngine PUBLIC ${flecs_SOURCE_DIR}/include)
  endif()
else()
  message(FATAL_ERROR "Flecs target not found.")
endif()

# ---- ImGui: add backends here, link vendored 'imgui' core ----
target_sources(NexusEngine PRIVATE
  ${CMAKE_SOURCE_DIR}/extern/imgui/backends/imgui_impl_sdl2.cpp
  ${CMAKE_SOURCE_DIR}/extern/DiligentEngine/DiligentTools/Imgui/src/ImGuiDiligentRenderer.cpp
  ${CMAKE_SOURCE_DIR}/extern/DiligentEngine/DiligentTools/Imgui/src/ImGuiImplDiligent.cpp
)

target_include_directories(NexusEngine PRIVATE
  ${CMAKE_SOURCE_DIR}/extern/imgui/backends
  ${CMAKE_SOURCE_DIR}/extern/DiligentEngine/DiligentCore/Common/interface
  ${CMAKE_SOURCE_DIR}/extern/DiligentEngine/DiligentCore/Graphics/GraphicsEngine/interface
  ${CMAKE_SOURCE_DIR}/extern/DiligentEngine/DiligentCore/Graphics/GraphicsTools/interface
  ${CMAKE_SOURCE_DIR}/extern/DiligentEngine/DiligentCore/Graphics/GraphicsAccessories/interface
  ${CMAKE_SOURCE_DIR}/extern/DiligentEngine/DiligentCore/Platforms/Basic/interface
  ${CMAKE_SOURCE_DIR}/extern/DiligentEngine/DiligentTools/Imgui/interface
)

target_link_libraries(NexusEngine PRIVATE imgui)

# SDL for the imgui_impl_sdl2 backend
if(TARGET NEXUS_SDL)
  target_link_libraries(NexusEngine PRIVATE NEXUS_SDL)
elseif(TARGET SDL2::SDL2)
  target_link_libraries(NexusEngine PRIVATE SDL2::SDL2 SDL2::SDL2main)
else()
  message(WARNING "SDL2 target not found; required for imgui_impl_sdl2 backend.")
endif()

if(MSVC)
  target_compile_options(NexusEngine PRIVATE /permissive- /W4 /EHsc /bigobj)
endif()
