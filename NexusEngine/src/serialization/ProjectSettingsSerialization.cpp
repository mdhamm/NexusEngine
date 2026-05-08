#include "ProjectSettingsSerialization.h"

#include "ISerializer.h"

#include <string>

namespace NexusEngine
{
    void SerializeProject(const ProjectSettings& project, ISerializeWriter& writer)
    {
        writer.Write("name", project.m_name);
        writer.Write("rootPath", project.m_rootPath.generic_string());
        writer.EndObject();
    }

    bool DeserializeProject(ProjectSettings& project, ISerializeReader& reader)
    {
        project = {};

        reader.Read("name", project.m_name);

        std::string rootPath;
        reader.Read("rootPath", rootPath);
        project.m_rootPath = rootPath;

        return !project.m_name.empty() && !rootPath.empty();
    }
} // namespace NexusEngine
