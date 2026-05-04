#include "AssetGuid.h"

#include <array>
#include <random>
#include <sstream>

namespace NexusEngine::IO
{
    std::string CreateAssetGuid()
    {
        std::random_device randomDevice;
        std::mt19937 generator(randomDevice());
        std::uniform_int_distribution<int> distribution(0, 255);

        std::array<unsigned char, 16> bytes{};
        for (unsigned char& value : bytes)
        {
            value = static_cast<unsigned char>(distribution(generator));
        }

        bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);
        bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);

        std::ostringstream stream;
        stream << std::hex;
        for (size_t index = 0; index < bytes.size(); ++index)
        {
            if (index == 4 || index == 6 || index == 8 || index == 10)
            {
                stream << '-';
            }

            stream.width(2);
            stream.fill('0');
            stream << static_cast<int>(bytes[index]);
        }

        return stream.str();
    }
}
