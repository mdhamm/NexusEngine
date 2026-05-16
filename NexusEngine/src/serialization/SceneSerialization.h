#pragma once

#include "NexusEngineApi.h"
#include "Scene.h"

namespace NexusEngine
{
    class ISerializeReader;
    class ISerializeWriter;

    NEXUS_ENGINE_API void Serialize(const Scene& scene, ISerializeWriter& writer);

    NEXUS_ENGINE_API bool Deserialize(Scene& scene, ISerializeReader& reader);

} // namespace NexusEngine
