#pragma once

#include "NexusEngineApi.h"

#include <string>

namespace NexusEngine::IO
{
    /// <summary>
    /// Creates a new asset guid string.
    /// </summary>
    /// <returns>A newly generated asset guid.</returns>
    NEXUS_ENGINE_API std::string CreateAssetGuid();
} // namespace NexusEngine::IO
