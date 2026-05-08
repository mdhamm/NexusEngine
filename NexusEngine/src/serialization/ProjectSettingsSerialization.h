#pragma once

#include "ProjectSettings.h"

namespace NexusEngine
{
    class ISerializeReader;
    class ISerializeWriter;

    void SerializeProject(const ProjectSettings& project, ISerializeWriter& writer);

    bool DeserializeProject(ProjectSettings& project, ISerializeReader& reader);
} // namespace NexusEngine
