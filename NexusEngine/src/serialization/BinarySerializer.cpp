#include "BinarySerializer.h"

namespace NexusEngine
{
    BinarySerializeWriter::BinarySerializeWriter(std::ostream& out)
        : m_out(out)
    {
    }

    void BinarySerializeWriter::BeginObject(std::string_view name)
    {
        WriteStringRaw(name);
    }

    void BinarySerializeWriter::EndObject()
    {
    }

    void BinarySerializeWriter::BeginArray(std::string_view name)
    {
        WriteStringRaw(name);
    }

    void BinarySerializeWriter::EndArray()
    {
    }

    void BinarySerializeWriter::BeginArrayElement()
    {
    }

    void BinarySerializeWriter::EndArrayElement()
    {
    }

    void BinarySerializeWriter::Write(std::string_view name, float value)
    {
        WriteStringRaw(name);
        WriteRaw(value);
    }

    void BinarySerializeWriter::Write(std::string_view name, bool value)
    {
        WriteStringRaw(name);
        WriteRaw(value);
    }

    void BinarySerializeWriter::Write(std::string_view name, int32_t value)
    {
        WriteStringRaw(name);
        WriteRaw(value);
    }

    void BinarySerializeWriter::Write(std::string_view name, uint32_t value)
    {
        WriteStringRaw(name);
        WriteRaw(value);
    }

    void BinarySerializeWriter::Write(std::string_view name, int64_t value)
    {
        WriteStringRaw(name);
        WriteRaw(value);
    }

    void BinarySerializeWriter::Write(std::string_view name, uint64_t value)
    {
        WriteStringRaw(name);
        WriteRaw(value);
    }

    void BinarySerializeWriter::Write(std::string_view name, std::string_view value)
    {
        WriteStringRaw(name);
        WriteStringRaw(value);
    }

    void BinarySerializeWriter::WriteStringRaw(std::string_view value) {
        uint32_t size = static_cast<uint32_t>(value.size());
        WriteRaw(size);

        if (size > 0) {
            m_out.write(value.data(), size);
        }
    }

    BinarySerializeReader::BinarySerializeReader(std::istream& in)
        : m_in(in)
    {
    }

    void BinarySerializeReader::BeginObject(std::string_view name)
    {
        ReadAndDiscardName(name);
    }

    void BinarySerializeReader::EndObject()
    {
    }

    void BinarySerializeReader::BeginArray(std::string_view name)
    {
        ReadAndDiscardName(name);
    }

    void BinarySerializeReader::EndArray()
    {
    }

    size_t BinarySerializeReader::GetArraySize(std::string_view name) const
    {
        // This simple binary format does not currently store array sizes.
        // Scene deserialization will need array sizes written explicitly
        // or a stronger binary container format.
        return 0;
    }

    void BinarySerializeReader::BeginArrayElement(size_t index)
    {
    }

    void BinarySerializeReader::EndArrayElement()
    {
    }

    std::vector<std::string> BinarySerializeReader::GetObjectKeys() const
    {
        // This simple binary format does not store object keys in a way
        // that supports discovery like JSON.
        return {};
    }

    void BinarySerializeReader::Read(std::string_view name, float& value)
    {
        ReadAndDiscardName(name);
        ReadRaw(value);
    }

    void BinarySerializeReader::Read(std::string_view name, bool& value)
    {
        ReadAndDiscardName(name);
        ReadRaw(value);
    }

    void BinarySerializeReader::Read(std::string_view name, int& value)
    {
        ReadAndDiscardName(name);
        ReadRaw(value);
    }

    void BinarySerializeReader::Read(std::string_view name, std::string& value)
    {
        ReadAndDiscardName(name);
        ReadStringRaw(value);
    }

    void BinarySerializeReader::ReadStringRaw(std::string& value) {
        uint32_t size = 0;
        ReadRaw(size);

        value.resize(size);

        if (size > 0) {
            m_in.read(value.data(), size);
        }
    }

    void BinarySerializeReader::ReadAndDiscardName(std::string_view expectedName) {
        std::string actualName;
        ReadStringRaw(actualName);

        // Optional later:
        // assert(actualName == expectedName);
    }
} // namespace NexusEngine