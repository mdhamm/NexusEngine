#include "ProjectSettingsIO.h"

#include "serialization/ProjectSettingsSerialization.h"

namespace NexusEngine::IO
{
    bool SaveProjectToFile(const ProjectSettings& project, const std::filesystem::path& filePath, FileFormat fileFormat)
    {
        return WriteStructuredFile(
            filePath,
            fileFormat,
            [&](ISerializeWriter& writer)
            {
                NexusEngine::SerializeProject(project, writer);
                return true;
            });
    }

    bool LoadProjectFromFile(ProjectSettings& project, const std::filesystem::path& filePath, FileFormat fileFormat)
    {
        return ReadStructuredFile(
            filePath,
            fileFormat,
            [&](ISerializeReader& reader)
            {
                return NexusEngine::DeserializeProject(project, reader);
            });
    }
} // namespace NexusEngine::IO
