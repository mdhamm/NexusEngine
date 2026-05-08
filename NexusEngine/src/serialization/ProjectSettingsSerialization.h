#pragma once

#include "ProjectSettings.h"

namespace NexusEngine
{
    class ISerializeReader;
    class ISerializeWriter;

    void Serialize(const ProjectSettings& project, NexusEngine::ISerializeWriter& writer);

    bool Deserialize(ProjectSettings& project, NexusEngine::ISerializeReader& reader);
} // namespace NexusEngine

