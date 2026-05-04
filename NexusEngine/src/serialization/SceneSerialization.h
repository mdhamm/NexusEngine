#pragma once

#include "Scene.h"

namespace NexusEngine
{
    class ISerializeReader;
    class ISerializeWriter;

    void SerializeScene(const Scene& scene, ISerializeWriter& writer);

    bool DeserializeScene(Scene& scene, ISerializeReader& reader);

} // namespace NexusEngine
