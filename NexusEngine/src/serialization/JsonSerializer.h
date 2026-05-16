#pragma once

#include "NexusEngineApi.h"
#include "ISerializer.h"

#include <nlohmann/json_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

namespace NexusEngine
{
    class NEXUS_ENGINE_API JsonSerializeWriter final : public ISerializeWriter
    {
    public:
        explicit JsonSerializeWriter(nlohmann::json& root);

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
        nlohmann::json& m_root;
        nlohmann::json* m_current = nullptr;
        std::stack<nlohmann::json*> m_stack;

        nlohmann::json* CreateObject(std::string_view name);
        nlohmann::json* CreateArray(std::string_view name);
    };

    class NEXUS_ENGINE_API JsonSerializeReader final : public ISerializeReader
    {
    public:
        explicit JsonSerializeReader(const nlohmann::json& root);

        void BeginObject(std::string_view name = {}) override;
        void EndObject() override;
        void BeginArray(std::string_view name = {}) override;
        void EndArray() override;
        size_t GetArraySize(std::string_view name) const override;
        void BeginArrayElement(size_t index) override;
        void EndArrayElement() override;
        std::vector<std::string> GetObjectKeys() const override;
        void Read(std::string_view name, float& value) override;
        void Read(std::string_view name, bool& value) override;
        void Read(std::string_view name, int32_t& value) override;
        void Read(std::string_view name, uint32_t& value) override;
        void Read(std::string_view name, int64_t& value) override;
        void Read(std::string_view name, uint64_t& value) override;
        void Read(std::string_view name, std::string& value) override;

    private:
        const nlohmann::json& m_root;
        const nlohmann::json* m_current = nullptr;
        std::stack<const nlohmann::json*> m_stack;

        const nlohmann::json& GetChild(std::string_view name) const;
        void ReadValue(std::string_view name, float& value) const;
        void ReadValue(std::string_view name, bool& value) const;
        void ReadValue(std::string_view name, int32_t& value) const;
        void ReadValue(std::string_view name, uint32_t& value) const;
        void ReadValue(std::string_view name, int64_t& value) const;
        void ReadValue(std::string_view name, uint64_t& value) const;
        void ReadValue(std::string_view name, std::string& value) const;
    };
}