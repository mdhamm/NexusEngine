cmake_minimum_required(VERSION 3.20)
project(SampleGame LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

if(BUILD_WEB)
  add_library(SampleGame STATIC
    src/Game.cpp
    src/Game.h
    src/GameComponents.h
    # your ECS registration, components, systems, content bootstrap, etc.
  )
else()
  add_library(SampleGame SHARED
    src/Game.cpp
    src/Game.h
    src/GameComponents.h
    # your ECS registration, components, systems, content bootstrap, etc.
  )
  target_compile_definitions(SampleGame PRIVATE SAMPLEGAME_EXPORTS)
endif()

target_include_directories(SampleGame PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Game depends on the engine
target_link_libraries(SampleGame PUBLIC NexusEngine)
