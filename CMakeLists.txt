cmake_minimum_required(VERSION 3.20)
project(NexusRoot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(NEXUS_WORKSPACE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "Nexus workspace root directory")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

function(prefix_folder_recursive dir prefix)
    get_property(targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(t IN LISTS targets)
        get_target_property(current_folder ${t} FOLDER)
        if(current_folder AND NOT current_folder STREQUAL "current_folder-NOTFOUND")
            set_target_properties(${t} PROPERTIES FOLDER "${prefix}/${current_folder}")
        else()
            set_target_properties(${t} PROPERTIES FOLDER "${prefix}")
        endif()
    endforeach()

    get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
    foreach(subdir IN LISTS subdirs)
        prefix_folder_recursive("${subdir}" "${prefix}")
    endforeach()
endfunction()

# ---------------- Platform mode ----------------
option(BUILD_WEB "Build for Web (Emscripten)" OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(BUILD_WEB ON)
endif()

if(NOT BUILD_WEB)
  option(NEXUS_BUILD_EDITOR "Build the Qt-based editor target" ON)
else()
  set(NEXUS_BUILD_EDITOR OFF)
endif()

option(NEXUS_EDITOR_MODE "Build NexusEngine for editor-hosted DLL mode" OFF)
if(NOT BUILD_WEB AND NEXUS_BUILD_EDITOR)
  set(NEXUS_EDITOR_MODE ON CACHE BOOL "Build NexusEngine for editor-hosted DLL mode" FORCE)
endif()

# ---------------- Diligent (extern) ----------------
# Keep only the backend we use on each platform.
if(BUILD_WEB)
  set(DILIGENT_NO_OPENGL     ON  CACHE BOOL "" FORCE)
else()
  set(DILIGENT_NO_OPENGL     ON  CACHE BOOL "" FORCE)
endif()
set(DILIGENT_NO_VULKAN       ON  CACHE BOOL "" FORCE)
set(DILIGENT_NO_DIRECT3D11   ON  CACHE BOOL "" FORCE)
if(BUILD_WEB)
  set(DILIGENT_NO_WEBGPU     OFF CACHE BOOL "" FORCE)
else()
  set(DILIGENT_NO_WEBGPU     ON  CACHE BOOL "" FORCE)
endif()
if(NOT BUILD_WEB)
  set(DILIGENT_NO_DIRECT3D12 OFF CACHE BOOL "" FORCE)   # enable D3D12 on desktop
endif()
set(DILIGENT_BUILD_SAMPLES   OFF CACHE BOOL "" FORCE)
set(DILIGENT_BUILD_TESTS     OFF CACHE BOOL "" FORCE)

# Bring in Diligent sources (expects extern/DiligentEngine in your tree)
add_subdirectory(extern/DiligentEngine)

#if(BUILD_WEB)
#  function(nexus_apply_webgpu_tint_workarounds dir)
#    get_property(targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
#    foreach(t IN LISTS targets)
#      if(t MATCHES "^(tint_|absl_)")
#        get_target_property(target_type ${t} TYPE)
#        if(target_type STREQUAL "INTERFACE_LIBRARY")
#          target_compile_options(${t} INTERFACE
#            "$<$<COMPILE_LANGUAGE:CXX>:-Wno-c2y-extensions>"
#            "$<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-pragma>"
#          )
#        else()
#          target_compile_options(${t} PRIVATE
#            "$<$<COMPILE_LANGUAGE:CXX>:-Wno-c2y-extensions>"
#            "$<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-pragma>"
#          )
#        endif()
#      endif()
#    endforeach()
#
#    get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
#    foreach(subdir IN LISTS subdirs)
#      nexus_apply_webgpu_tint_workarounds("${subdir}")
#    endforeach()
#  endfunction()
#
#  nexus_apply_webgpu_tint_workarounds("${CMAKE_CURRENT_SOURCE_DIR}/extern/DiligentEngine")
#endif()

# ---- Flecs (ECS) ----
if(BUILD_WEB)
  set(FLECS_STATIC ON CACHE BOOL "" FORCE)
  set(FLECS_SHARED OFF CACHE BOOL "" FORCE)
elseif(NEXUS_EDITOR_MODE)
  set(FLECS_STATIC OFF CACHE BOOL "" FORCE)
  set(FLECS_SHARED ON CACHE BOOL "" FORCE)
else()
  set(FLECS_STATIC ON CACHE BOOL "" FORCE)
  set(FLECS_SHARED OFF CACHE BOOL "" FORCE)
endif()
set(FLECS_PIC      ON  CACHE BOOL "" FORCE)
set(FLECS_TESTS    OFF CACHE BOOL "" FORCE)
set(FLECS_DOCS     OFF CACHE BOOL "" FORCE)
set(FLECS_SANITIZE OFF CACHE BOOL "" FORCE)
set(FLECS_META     ON  CACHE BOOL "" FORCE)
add_subdirectory(extern/flecs)
if(TARGET flecs_static)
  set_target_properties(flecs_static PROPERTIES FOLDER "extern")
endif()
if(TARGET flecs)
  set_target_properties(flecs PROPERTIES FOLDER "extern")
endif()

if(BUILD_WEB)
  if(TARGET flecs_static)
    target_compile_definitions(flecs_static PUBLIC FLECS_CPP_ENUM_REFLECTION_SUPPORT=0)
    target_compile_options(flecs_static PRIVATE
      "-pthread"
      "-mbulk-memory"
    )
  endif()

  if(TARGET flecs)
    target_compile_definitions(flecs PUBLIC FLECS_CPP_ENUM_REFLECTION_SUPPORT=0)
    target_compile_options(flecs PRIVATE
      "-pthread"
      "-mbulk-memory"
    )
    target_link_options(flecs PRIVATE "-pthread")
  endif()
endif()

# ---- ImGui (docking) ----
#add_subdirectory(extern/imgui) 
#set_target_properties(imgui PROPERTIES FOLDER "extern")

# ---------------- SDL2 (shared iface for apps) ----------------
if(NOT BUILD_WEB)
  include(FetchContent)
  # Prefer static SDL on desktop (optional)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON  CACHE BOOL "" FORCE)

  FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG release-2.30.8
    FIND_PACKAGE_ARGS
  )
  FetchContent_MakeAvailable(SDL2)      # provides SDL2::SDL2main (+ SDL2::SDL2main)

  add_library(NEXUS_SDL INTERFACE)
  target_link_libraries(NEXUS_SDL INTERFACE SDL2::SDL2main SDL2::SDL2)
else()
  # Emscripten SDL2 port
  add_library(NEXUS_SDL INTERFACE)
  target_compile_options(NEXUS_SDL INTERFACE "-sUSE_SDL=2")
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

prefix_folder_recursive("${CMAKE_CURRENT_SOURCE_DIR}/extern/DiligentEngine"  "extern/Diligent")

# ---------------- Subprojects ----------------
# ImGui/Flecs/SDL/Diligent are all available BEFORE NexusEngine,
# so the engine can include <imgui.h> and link its backends.
add_subdirectory(NexusEngine)
if(NOT BUILD_WEB)
  if(NEXUS_BUILD_EDITOR)
    if(DEFINED Qt6_DIR AND MSVC AND NOT Qt6_DIR MATCHES "[\\/]msvc")
      message(WARNING "Ignoring incompatible Qt6_DIR='${Qt6_DIR}'. MSVC builds require a Qt6 msvc kit.")
      unset(Qt6_DIR CACHE)
      unset(Qt6_DIR)
    endif()

    if(NOT DEFINED Qt6_DIR AND EXISTS "C:/Qt")
      file(GLOB NEXUS_QT6_CONFIGS "C:/Qt/*/msvc*/lib/cmake/Qt6/Qt6Config.cmake")
      if(NEXUS_QT6_CONFIGS)
        list(SORT NEXUS_QT6_CONFIGS ORDER DESCENDING)
        list(GET NEXUS_QT6_CONFIGS 0 NEXUS_QT6_CONFIG)
        get_filename_component(Qt6_DIR "${NEXUS_QT6_CONFIG}" DIRECTORY)
        set(Qt6_DIR "${Qt6_DIR}" CACHE PATH "Path to Qt6 CMake package" FORCE)
      endif()
    endif()

    if(DEFINED Qt6_DIR)
      add_subdirectory(Editor)  # EXE
    else()
      message(WARNING "Skipping Editor target because a Qt6 MSVC package was not found. Install one under C:/Qt/<version>/msvc*_64 or set Qt6_DIR.")
    endif()
  endif()
endif()
