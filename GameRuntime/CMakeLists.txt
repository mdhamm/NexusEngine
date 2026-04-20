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
  # Configure the HTML template with CMake (like Diligent does)
  # Emscripten outputs .js by default, our HTML loads that JS
  set(TARGET_NAME "GameRuntime")
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/shell.html.in
    ${CMAKE_CURRENT_BINARY_DIR}/GameRuntime.html
    @ONLY
  )
  target_link_options(GameRuntime PRIVATE
    "-sUSE_SDL=2"
    "-sASSERTIONS=0"
    "-sUSE_WEBGPU=1"
    "-sUSE_PTHREADS=1"
    "-sPTHREAD_POOL_SIZE=2"
    "-sWASM_BIGINT"
    "-sWASM=1"
    "--preload-file=${CMAKE_SOURCE_DIR}/NexusEngine/shaders@/NexusEngine/shaders"
    "-sSTACK_SIZE=524288"
    "-sALLOW_MEMORY_GROWTH"
    "-sINITIAL_MEMORY=67108864"
    "-flto"
    "-O3"
  )

  # Pre-compress large web assets with Brotli at build time
  find_program(BROTLI_EXECUTABLE brotli)
  if(BROTLI_EXECUTABLE)
    add_custom_command(TARGET GameRuntime POST_BUILD
      COMMAND ${BROTLI_EXECUTABLE} -f -k -q 11 "$<TARGET_FILE_DIR:GameRuntime>/GameRuntime.wasm"
      COMMAND ${BROTLI_EXECUTABLE} -f -k -q 11 "$<TARGET_FILE_DIR:GameRuntime>/GameRuntime.js"
      COMMAND ${BROTLI_EXECUTABLE} -f -k -q 11 "$<TARGET_FILE_DIR:GameRuntime>/GameRuntime.data"
      COMMENT "Pre-compressing web assets with Brotli..."
      VERBATIM
    )
  else()
    message(WARNING "Brotli not found - install with 'npm install -g brotli' or 'choco install brotli' for faster web serving")
  endif()
else()
  target_link_libraries(GameRuntime PRIVATE DILIGENT_D3D12 NEXUS_SDL SampleGame)
  if(MSVC)
    target_compile_options(GameRuntime PRIVATE /permissive- /W4 /EHsc /bigobj)
  endif()
endif()
