// BinarySerializer.h
#pragma once

#include "ISerializer.h"

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
// TODO: move to cpp
namespace NexusEngine
{
    class BinarySerializeWriter final : public ISerializeWriter {
    public:
        explicit BinarySerializeWriter(std::ostream& out)
            : m_out(out) {
        }

        void BeginObject(std::string_view name = {}) override {
            WriteStringRaw(name);
        }

        void EndObject() override {
        }

        void BeginArray(std::string_view name = {}) override {
            WriteStringRaw(name);
        }

        void EndArray() override {
        }

        void BeginArrayElement() override {
        }

        void EndArrayElement() override {
        }

        void Write(std::string_view name, float value) override {
            WriteStringRaw(name);
            WriteRaw(value);
        }

        void Write(std::string_view name, bool value) override {
            WriteStringRaw(name);
            WriteRaw(value);
        }

        void Write(std::string_view name, int32_t value) override {
            WriteStringRaw(name);
            WriteRaw(value);
        }

        void Write(std::string_view name, uint32_t value) override {
            WriteStringRaw(name);
            WriteRaw(value);
        }

        void Write(std::string_view name, int64_t value) override {
            WriteStringRaw(name);
            WriteRaw(value);
        }

        void Write(std::string_view name, uint64_t value) override {
            WriteStringRaw(name);
            WriteRaw(value);
        }

        void Write(std::string_view name, std::string_view value) override {
            WriteStringRaw(name);
            WriteStringRaw(value);
        }

    private:
        std::ostream& m_out;

        template<typename T>
        void WriteRaw(const T& value) {
            m_out.write(reinterpret_cast<const char*>(&value), sizeof(T));
        }

        void WriteStringRaw(std::string_view value) {
            uint32_t size = static_cast<uint32_t>(value.size());
            WriteRaw(size);

            if (size > 0) {
                m_out.write(value.data(), size);
            }
        }
    };

    class BinarySerializeReader final : public ISerializeReader {
    public:
        explicit BinarySerializeReader(std::istream& in)
            : m_in(in) {
        }

        void BeginObject(std::string_view name = {}) override {
            ReadAndDiscardName(name);
        }

        void EndObject() override {
        }

        void BeginArray(std::string_view name = {}) override {
            ReadAndDiscardName(name);
        }

        void EndArray() override {
        }

        size_t GetArraySize(std::string_view name) const override {
            // This simple binary format does not currently store array sizes.
            // Scene deserialization will need array sizes written explicitly
            // or a stronger binary container format.
            return 0;
        }

        void BeginArrayElement(size_t index) override {
        }

        void EndArrayElement() override {
        }

        std::vector<std::string> GetObjectKeys() const override {
            // This simple binary format does not store object keys in a way
            // that supports discovery like JSON.
            return {};
        }

        void Read(std::string_view name, float& value) override {
            ReadAndDiscardName(name);
            ReadRaw(value);
        }

        void Read(std::string_view name, bool& value) override {
            ReadAndDiscardName(name);
            ReadRaw(value);
        }

        void Read(std::string_view name, int& value) override {
            ReadAndDiscardName(name);
            ReadRaw(value);
        }

        void Read(std::string_view name, std::string& value) override {
            ReadAndDiscardName(name);
            ReadStringRaw(value);
        }

    private:
        std::istream& m_in;

        template<typename T>
        void ReadRaw(T& value) {
            m_in.read(reinterpret_cast<char*>(&value), sizeof(T));
        }

        void ReadStringRaw(std::string& value) {
            uint32_t size = 0;
            ReadRaw(size);

            value.resize(size);

            if (size > 0) {
                m_in.read(value.data(), size);
            }
        }

        void ReadAndDiscardName(std::string_view expectedName) {
            std::string actualName;
            ReadStringRaw(actualName);

            // Optional later:
            // assert(actualName == expectedName);
        }
    };
}