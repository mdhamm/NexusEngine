#include "JsonSerializer.h"

#include <nlohmann/json.hpp>

namespace NexusEngine
{
    JsonSerializeWriter::JsonSerializeWriter(nlohmann::json& root)
        : m_root(root)
        , m_current(&root)
    {
        m_root = nlohmann::json::object();
    }

    void JsonSerializeWriter::BeginObject(std::string_view name)
    {
        nlohmann::json* object = CreateObject(name);
        m_stack.push(m_current);
        m_current = object;
    }

    void JsonSerializeWriter::EndObject()
    {
        m_current = m_stack.top();
        m_stack.pop();
    }

    void JsonSerializeWriter::BeginArray(std::string_view name)
    {
        nlohmann::json* array = CreateArray(name);
        m_stack.push(m_current);
        m_current = array;
    }

    void JsonSerializeWriter::EndArray()
    {
        m_current = m_stack.top();
        m_stack.pop();
    }

    void JsonSerializeWriter::BeginArrayElement()
    {
        m_current->push_back(nlohmann::json::object());
        m_stack.push(m_current);
        m_current = &m_current->back();
    }

    void JsonSerializeWriter::EndArrayElement()
    {
        m_current = m_stack.top();
        m_stack.pop();
    }

    void JsonSerializeWriter::Write(std::string_view name, float value)
    {
        (*m_current)[std::string(name)] = value;
    }

    void JsonSerializeWriter::Write(std::string_view name, bool value)
    {
        (*m_current)[std::string(name)] = value;
    }

    void JsonSerializeWriter::Write(std::string_view name, int32_t value)
    {
        (*m_current)[std::string(name)] = value;
    }

    void JsonSerializeWriter::Write(std::string_view name, uint32_t value)
    {
        (*m_current)[std::string(name)] = value;
    }

    void JsonSerializeWriter::Write(std::string_view name, int64_t value)
    {
        (*m_current)[std::string(name)] = value;
    }

    void JsonSerializeWriter::Write(std::string_view name, uint64_t value)
    {
        (*m_current)[std::string(name)] = value;
    }

    void JsonSerializeWriter::Write(std::string_view name, std::string_view value)
    {
        (*m_current)[std::string(name)] = std::string(value);
    }

    nlohmann::json* JsonSerializeWriter::CreateObject(std::string_view name)
    {
        if (name.empty())
        {
            *m_current = nlohmann::json::object();
            return m_current;
        }

        const std::string key(name);
        (*m_current)[key] = nlohmann::json::object();
        return &(*m_current)[key];
    }

    nlohmann::json* JsonSerializeWriter::CreateArray(std::string_view name)
    {
        if (name.empty())
        {
            *m_current = nlohmann::json::array();
            return m_current;
        }

        const std::string key(name);
        (*m_current)[key] = nlohmann::json::array();
        return &(*m_current)[key];
    }

    JsonSerializeReader::JsonSerializeReader(const nlohmann::json& root)
        : m_root(root)
        , m_current(&root)
    {
    }

    void JsonSerializeReader::BeginObject(std::string_view name)
    {
        const nlohmann::json& object = GetChild(name);
        m_stack.push(m_current);
        m_current = &object;
    }

    void JsonSerializeReader::EndObject()
    {
        m_current = m_stack.top();
        m_stack.pop();
    }

    void JsonSerializeReader::BeginArray(std::string_view name)
    {
        const nlohmann::json& array = GetChild(name);
        m_stack.push(m_current);
        m_current = &array;
    }

    void JsonSerializeReader::EndArray()
    {
        m_current = m_stack.top();
        m_stack.pop();
    }

    size_t JsonSerializeReader::GetArraySize(std::string_view name) const
    {
        const nlohmann::json& array = name.empty()
            ? *m_current
            : m_current->at(std::string(name));

        return array.size();
    }

    void JsonSerializeReader::BeginArrayElement(size_t index)
    {
        m_stack.push(m_current);
        m_current = &m_current->at(index);
    }

    void JsonSerializeReader::EndArrayElement()
    {
        m_current = m_stack.top();
        m_stack.pop();
    }

    std::vector<std::string> JsonSerializeReader::GetObjectKeys() const
    {
        std::vector<std::string> keys;

        if (!m_current->is_object())
        {
            return keys;
        }

        for (auto it = m_current->begin(); it != m_current->end(); ++it)
        {
            keys.push_back(it.key());
        }

        return keys;
    }

    void JsonSerializeReader::Read(std::string_view name, float& value)
    {
        ReadValue(name, value);
    }

    void JsonSerializeReader::Read(std::string_view name, bool& value)
    {
        ReadValue(name, value);
    }

    void JsonSerializeReader::Read(std::string_view name, int32_t& value)
    {
        ReadValue(name, value);
    }

    void JsonSerializeReader::Read(std::string_view name, uint32_t& value)
    {
        ReadValue(name, value);
    }

    void JsonSerializeReader::Read(std::string_view name, int64_t& value)
    {
        ReadValue(name, value);
    }

    void JsonSerializeReader::Read(std::string_view name, uint64_t& value)
    {
        ReadValue(name, value);
    }

    void JsonSerializeReader::Read(std::string_view name, std::string& value)
    {
        ReadValue(name, value);
    }

    const nlohmann::json& JsonSerializeReader::GetChild(std::string_view name) const
    {
        if (name.empty())
        {
            return *m_current;
        }

        return m_current->at(std::string(name));
    }

    void JsonSerializeReader::ReadValue(std::string_view name, float& value) const
    {
        const std::string key(name);
        if (!m_current->contains(key))
        {
            return;
        }

        value = (*m_current)[key].get<float>();
    }

    void JsonSerializeReader::ReadValue(std::string_view name, bool& value) const
    {
        const std::string key(name);
        if (!m_current->contains(key))
        {
            return;
        }

        value = (*m_current)[key].get<bool>();
    }

    void JsonSerializeReader::ReadValue(std::string_view name, int32_t& value) const
    {
        const std::string key(name);
        if (!m_current->contains(key))
        {
            return;
        }

        value = (*m_current)[key].get<int32_t>();
    }

    void JsonSerializeReader::ReadValue(std::string_view name, uint32_t& value) const
    {
        const std::string key(name);
        if (!m_current->contains(key))
        {
            return;
        }

        value = (*m_current)[key].get<uint32_t>();
    }

    void JsonSerializeReader::ReadValue(std::string_view name, int64_t& value) const
    {
        const std::string key(name);
        if (!m_current->contains(key))
        {
            return;
        }

        value = (*m_current)[key].get<int64_t>();
    }

    void JsonSerializeReader::ReadValue(std::string_view name, uint64_t& value) const
    {
        const std::string key(name);
        if (!m_current->contains(key))
        {
            return;
        }

        value = (*m_current)[key].get<uint64_t>();
    }

    void JsonSerializeReader::ReadValue(std::string_view name, std::string& value) const
    {
        const std::string key(name);
        if (!m_current->contains(key))
        {
            return;
        }

        value = (*m_current)[key].get<std::string>();
    }
}
