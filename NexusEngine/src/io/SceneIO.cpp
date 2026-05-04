#include "SceneIO.h"

#include "FileIO.h"

namespace NexusEngine::IO
{
    bool SaveSceneToFile(
        const Scene& scene,
        const std::filesystem::path& filePath,
        const SerializeSceneMethod& serializeMethod,
        const CreateSceneWriterMethod& createWriter)
    {
        if (!serializeMethod || !createWriter)
        {
            return false;
        }

        return WriteSerializedFile(
            filePath,
            [&](std::vector<std::uint8_t>& bytes)
            {
                return createWriter(bytes, scene, serializeMethod);
            });
    }

    bool LoadSceneFromFile(
        Scene& scene,
        const std::filesystem::path& filePath,
        const DeserializeSceneMethod& deserializeMethod,
        const CreateSceneReaderMethod& createReader)
    {
        if (!deserializeMethod || !createReader)
        {
            return false;
        }

        return ReadSerializedFile(
            filePath,
            [&](const std::vector<std::uint8_t>& bytes)
            {
                return createReader(bytes, scene, deserializeMethod);
            });
    }
} // namespace NexusEngine::IO
