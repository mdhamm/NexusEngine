cmake_minimum_required(VERSION 3.20)
project(GameRuntime LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

option(BUILD_WEB "Build for Web (Emscripten)" OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(BUILD_WEB ON)
endif()

if(BUILD_WEB)
  set(GAME_MAIN src/main_web.cpp)
else()
  set(GAME_MAIN src/main_win32.cpp)
endif()

add_executable(GameRuntime ${GAME_MAIN})
target_link_libraries(GameRuntime PRIVATE SampleGame NEXUS_SDL)

if(BUILD_WEB)
  set_target_properties(GameRuntime PROPERTIES SUFFIX ".html")
  target_link_options(GameRuntime PRIVATE
    "-sUSE_SDL=2"
    "-sASSERTIONS=1"
    "-sUSE_WEBGPU=1"
    "-sASYNCIFY"
    "--preload-file=${CMAKE_SOURCE_DIR}/NexusEngine/shaders@/NexusEngine/shaders"
    "-sSTACK_SIZE=524288"
    "-sALLOW_MEMORY_GROWTH"
  )
else()
  target_link_libraries(GameRuntime PRIVATE DILIGENT_D3D12 NEXUS_SDL SampleGame)
  if(MSVC)
    target_compile_options(GameRuntime PRIVATE /permissive- /W4 /EHsc /bigobj)
  endif()
endif()
