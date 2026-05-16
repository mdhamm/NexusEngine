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
  set(NEXUS_EDITOR_PREFERRED_QT6_DIR "C:/Qt/6.11.0/msvc2022_64/lib/cmake/Qt6")

  if(NOT DEFINED Qt6_DIR AND EXISTS "${NEXUS_EDITOR_PREFERRED_QT6_DIR}/Qt6Config.cmake")
    set(Qt6_DIR "${NEXUS_EDITOR_PREFERRED_QT6_DIR}" CACHE PATH "Path to Qt6 CMake package" FORCE)
  endif()

  if(NOT DEFINED Qt6_DIR AND EXISTS "C:/Qt")
    file(GLOB QT6_CONFIGS "C:/Qt/*/msvc*/lib/cmake/Qt6/Qt6Config.cmake")
    if(QT6_CONFIGS)
      list(SORT QT6_CONFIGS ORDER DESCENDING)
      list(GET QT6_CONFIGS 0 QT6_CONFIG)
      get_filename_component(Qt6_DIR "${QT6_CONFIG}" DIRECTORY)
      set(Qt6_DIR "${Qt6_DIR}" CACHE PATH "Path to Qt6 CMake package" FORCE)
    else()
      message(FATAL_ERROR "Qt6 MSVC package not found under C:/Qt. Install a Qt6 msvc kit such as C:/Qt/<version>/msvc2022_64.")
    endif()
  endif()

  message(STATUS "Editor configuring with Qt6_DIR='${Qt6_DIR}'")
  find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets)

  get_filename_component(NEXUS_EDITOR_QT_CMAKE_DIR "${Qt6_DIR}" ABSOLUTE)
  get_filename_component(NEXUS_EDITOR_QT_LIB_CMAKE_DIR "${NEXUS_EDITOR_QT_CMAKE_DIR}" DIRECTORY)
  get_filename_component(NEXUS_EDITOR_QT_LIB_DIR "${NEXUS_EDITOR_QT_LIB_CMAKE_DIR}" DIRECTORY)
  get_filename_component(NEXUS_EDITOR_QT_ROOT_DIR "${NEXUS_EDITOR_QT_LIB_DIR}" DIRECTORY)
  find_program(NEXUS_EDITOR_WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt windeployqt.exe
    HINTS "${NEXUS_EDITOR_QT_ROOT_DIR}/bin")

  set(GAME_MAIN
    src/main_win32.cpp
    src/widgets/ContentDrawerWidget.cpp
    src/widgets/ContentDrawerWidget.h
    src/EditorProject.h
    src/EditorProjectRegistry.cpp
    src/EditorProjectRegistry.h
    
    src/EditorSceneSerializer.cpp
    src/EditorSceneSerializer.h
    src/FileExtensionInfo.cpp
    src/FileExtensionInfo.h
    "src/windows/EditorWindow.cpp"
    "src/windows/EditorWindow.h"
    src/widgets/PropertyWidget.cpp
    src/widgets/PropertyWidget.h
    src/widgets/PropertyWidgetSerializer.cpp
    src/widgets/PropertyWidgetSerializer.h
    "src/windows/ProjectBrowserWindow.cpp"
    "src/windows/ProjectBrowserWindow.h"
    src/widgets/SceneGraphWidget.cpp
    src/widgets/SceneGraphWidget.h
    src/widgets/SceneViewWidget.cpp
    src/widgets/SceneViewWidget.h
  )
endif()

add_executable(Editor ${GAME_MAIN})

target_link_libraries(Editor PRIVATE NexusEngine)

target_include_directories(Editor PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets
  ${CMAKE_CURRENT_SOURCE_DIR}/src/windows
)

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
  target_link_libraries(Editor PRIVATE DILIGENT_D3D12 NEXUS_SDL Qt6::Widgets)
  target_compile_definitions(Editor PRIVATE NEXUS_EDITOR_CONTENT_ROOT="${CMAKE_SOURCE_DIR}")

  if(WIN32 AND NEXUS_EDITOR_WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET Editor POST_BUILD
      COMMAND "${NEXUS_EDITOR_WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime "$<TARGET_FILE:Editor>"
      COMMENT "Deploying Qt runtime for Editor")
  endif()

  if(WIN32)
    add_custom_command(TARGET Editor POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:Editor> $<TARGET_FILE_DIR:Editor>
      COMMAND_EXPAND_LISTS
      COMMENT "Copying runtime DLLs for Editor")
  endif()

  if(MSVC)
    target_compile_options(Editor PRIVATE /permissive- /Zc:__cplusplus /W4 /EHsc /bigobj)
  endif()
endif()
