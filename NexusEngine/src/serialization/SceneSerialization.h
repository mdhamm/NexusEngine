#pragma once

#include "Scene.h"

namespace NexusEngine
{
    class ISerializeReader;
    class ISerializeWriter;

    void Serialize(const Scene& scene, ISerializeWriter& writer);

    bool Deserialize(Scene& scene, ISerializeReader& reader);

} // namespace NexusEngine
