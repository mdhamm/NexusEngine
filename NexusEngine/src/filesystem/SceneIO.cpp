#include "SceneIO.h"

#include "FileIO.h"
#include "serialization/SceneSerialization.h"

namespace NexusEngine::IO
{
    bool SaveSceneToFile(const Scene& scene, const std::filesystem::path& filePath, FileFormat fileFormat)
    {
        return WriteStructuredFile(
            filePath,
            fileFormat,
            [&](ISerializeWriter& writer)
            {
                SerializeScene(scene, writer);
                return true;
            });
    }

    bool LoadSceneFromFile(Scene& scene, const std::filesystem::path& filePath, FileFormat fileFormat)
    {
        return ReadStructuredFile(
            filePath,
            fileFormat,
            [&](ISerializeReader& reader)
            {
                return DeserializeScene(scene, reader);
            });
    }
} // namespace NexusEngine::IO
