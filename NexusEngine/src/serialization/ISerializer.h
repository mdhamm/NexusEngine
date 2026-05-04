// ISerializer.h
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace NexusEngine
{
    class ISerializeReader {
    public:
        virtual ~ISerializeReader() = default;

        virtual void BeginObject(std::string_view name = {}) = 0;
        virtual void EndObject() = 0;

        virtual void BeginArray(std::string_view name = {}) = 0;
        virtual void EndArray() = 0;

        virtual std::size_t GetArraySize(std::string_view name = {}) const = 0;

        virtual void BeginArrayElement(std::size_t index) = 0;
        virtual void EndArrayElement() = 0;

        virtual std::vector<std::string> GetObjectKeys() const = 0;

        virtual void Read(std::string_view name, float& value) = 0;
        virtual void Read(std::string_view name, bool& value) = 0;
        virtual void Read(std::string_view name, int32_t& value) = 0;
        virtual void Read(std::string_view name, uint32_t& value) = 0;
        virtual void Read(std::string_view name, int64_t& value) = 0;
        virtual void Read(std::string_view name, uint64_t& value) = 0;
        virtual void Read(std::string_view name, std::string& value) = 0;
    };

    class ISerializeWriter {
    public:
        virtual ~ISerializeWriter() = default;

        virtual void BeginObject(std::string_view name = {}) = 0;
        virtual void EndObject() = 0;

        virtual void BeginArray(std::string_view name = {}) = 0;
        virtual void EndArray() = 0;

        virtual void BeginArrayElement() = 0;
        virtual void EndArrayElement() = 0;

        virtual void Write(std::string_view name, float value) = 0;
        virtual void Write(std::string_view name, bool value) = 0;
        virtual void Write(std::string_view name, int32_t value) = 0;
        virtual void Write(std::string_view name, uint32_t value) = 0;
        virtual void Write(std::string_view name, int64_t value) = 0;
        virtual void Write(std::string_view name, uint64_t value) = 0;
        virtual void Write(std::string_view name, std::string_view value) = 0;
    };
}