#pragma once

#include "ISerializer.h"

#include <cstdint>
#include <istream>
#include <ostream>
#include <string_view>

namespace NexusEngine
{
    class BinarySerializeWriter final : public ISerializeWriter {
    public:
        explicit BinarySerializeWriter(std::ostream& out);

        void BeginObject(std::string_view name = {}) override;
        void EndObject() override;
        void BeginArray(std::string_view name = {}) override;
        void EndArray() override;
        void BeginArrayElement() override;
        void EndArrayElement() override;
        void Write(std::string_view name, float value) override;
        void Write(std::string_view name, bool value) override;
        void Write(std::string_view name, int32_t value) override;
        void Write(std::string_view name, uint32_t value) override;
        void Write(std::string_view name, int64_t value) override;
        void Write(std::string_view name, uint64_t value) override;
        void Write(std::string_view name, std::string_view value) override;

    private:
        std::ostream& m_out;

        template<typename T>
        void WriteRaw(const T& value) {
            m_out.write(reinterpret_cast<const char*>(&value), sizeof(T));
        }

        void WriteStringRaw(std::string_view value);
    };

    class BinarySerializeReader final : public ISerializeReader {
    public:
        explicit BinarySerializeReader(std::istream& in);

        void BeginObject(std::string_view name = {}) override;
        void EndObject() override;
        void BeginArray(std::string_view name = {}) override;
        void EndArray() override;
        size_t GetArraySize(std::string_view name) const override;
        void BeginArrayElement(size_t index) override;
        void EndArrayElement() override;
        std::vector<std::string> GetObjectKeys() const;
        void Read(std::string_view name, float& value) override;
        void Read(std::string_view name, bool& value) override;
        void Read(std::string_view name, int& value) override;
        void Read(std::string_view name, std::string& value) override;

    private:
        std::istream& m_in;

        template<typename T>
        void ReadRaw(T& value) {
            m_in.read(reinterpret_cast<char*>(&value), sizeof(T));
        }

        void ReadStringRaw(std::string& value);
        void ReadAndDiscardName(std::string_view expectedName);
    };
} // namespace NexusEngine