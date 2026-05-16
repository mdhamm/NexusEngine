#pragma once

#include "NexusEngineApi.h"
#include "ProjectSettings.h"

namespace NexusEngine
{
    class ISerializeReader;
    class ISerializeWriter;

    NEXUS_ENGINE_API void Serialize(const ProjectSettings& project, NexusEngine::ISerializeWriter& writer);

    NEXUS_ENGINE_API bool Deserialize(ProjectSettings& project, NexusEngine::ISerializeReader& reader);
} // namespace NexusEngine

