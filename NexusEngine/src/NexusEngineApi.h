#pragma once

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
#if defined(NEXUS_ENGINE_EXPORTS)
#define NEXUS_ENGINE_API __declspec(dllexport)
#elif defined(NEXUS_EDITOR_MODE)
#define NEXUS_ENGINE_API __declspec(dllimport)
#else
#define NEXUS_ENGINE_API
#endif
#else
#define NEXUS_ENGINE_API
#endif
