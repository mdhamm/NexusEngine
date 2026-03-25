cmake_minimum_required(VERSION 3.20)
project(Editor LANGUAGES CXX)
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

add_executable(Editor ${GAME_MAIN})
target_link_libraries(Editor PRIVATE SampleGame)

if(BUILD_WEB)
  target_link_libraries(Editor PRIVATE DILIGENT_WEBGPU)
  set_target_properties(Editor PROPERTIES SUFFIX ".html")
  target_link_options(Editor PRIVATE
    "SHELL:--use-port=emdawnwebgpu"   # or comment and use -sUSE_WEBGPU=1 on older SDKs
    "-sUSE_SDL=2"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sASSERTIONS=1"
  )
else()
  target_link_libraries(Editor PRIVATE DILIGENT_D3D12 NEXUS_SDL)
  if(MSVC)
    target_compile_options(Editor PRIVATE /permissive- /W4 /EHsc /bigobj)
  endif()
endif()
