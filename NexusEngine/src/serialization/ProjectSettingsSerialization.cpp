#include "ProjectSettingsSerialization.h"

#include "ISerializer.h"

#include <string>

namespace NexusEngine
{
    void Serialize(const ProjectSettings& project, NexusEngine::ISerializeWriter& writer)
    {
        writer.Write("name", project.m_name);
        writer.Write("defaultScene", project.m_defaultScene.m_guid);
        writer.EndObject();
    }

    bool Deserialize(ProjectSettings& project, NexusEngine::ISerializeReader& reader)
    {
        project = {};

        reader.Read("name", project.m_name);
        reader.Read("defaultScene", project.m_defaultScene.m_guid);

        return !project.m_name.empty();
    }
} // namespace NexusEngine
