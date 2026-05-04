#pragma once

#include "ISerializer.h"
#include <nlohmann/json.hpp>

#include <stack>
#include <string>
#include <string_view>
#include <vector>
#include <cstddef>
// TODO: move to cpp
namespace NexusEngine
{
    class JsonSerializeReader final : public ISerializeReader {
    public:
        explicit JsonSerializeReader(const nlohmann::json& root)
            : m_root(root), m_current(&root)
        {
        }

        void BeginObject(std::string_view name = {}) override {
            const nlohmann::json& obj = GetChild(name);
            m_stack.push(m_current);
            m_current = &obj;
        }

        void EndObject() override {
            m_current = m_stack.top();
            m_stack.pop();
        }

        void BeginArray(std::string_view name = {}) override {
            const nlohmann::json& arr = GetChild(name);
            m_stack.push(m_current);
            m_current = &arr;
        }

        void EndArray() override {
            m_current = m_stack.top();
            m_stack.pop();
        }

        size_t GetArraySize(std::string_view name) const override {
            const nlohmann::json& arr = name.empty()
                ? *m_current
                : m_current->at(std::string(name));

            return arr.size();
        }

        void BeginArrayElement(size_t index) override {
            m_stack.push(m_current);
            m_current = &m_current->at(index);
        }

        void EndArrayElement() override {
            m_current = m_stack.top();
            m_stack.pop();
        }

        std::vector<std::string> GetObjectKeys() const override {
            std::vector<std::string> keys;

            if (!m_current->is_object())
                return keys;

            for (auto it = m_current->begin(); it != m_current->end(); ++it) {
                keys.push_back(it.key());
            }

            return keys;
        }

        void Read(std::string_view name, float& value) override {
            ReadValue(name, value);
        }

        void Read(std::string_view name, bool& value) override {
            ReadValue(name, value);
        }

        void Read(std::string_view name, int& value) override {
            ReadValue(name, value);
        }

        void Read(std::string_view name, std::string& value) override {
            ReadValue(name, value);
        }

    private:
        const nlohmann::json& m_root;
        const nlohmann::json* m_current = nullptr;
        std::stack<const nlohmann::json*> m_stack;

        const nlohmann::json& GetChild(std::string_view name) const {
            if (name.empty())
                return *m_current;

            return m_current->at(std::string(name));
        }

        template<typename T>
        void ReadValue(std::string_view name, T& value) const {
            auto key = std::string(name);

            if (!m_current->contains(key))
                return;

            value = (*m_current)[key].get<T>();
        }
    };

    class JsonSerializeWriter final : public ISerializeWriter {
    public:
        explicit JsonSerializeWriter(nlohmann::json& root)
            : m_root(root), m_current(&root)
        {
            m_root = nlohmann::json::object();
        }

        void BeginObject(std::string_view name = {}) override {
            nlohmann::json* obj = CreateObject(name);
            m_stack.push(m_current);
            m_current = obj;
        }

        void EndObject() override {
            m_current = m_stack.top();
            m_stack.pop();
        }

        void BeginArray(std::string_view name = {}) override {
            nlohmann::json* arr = CreateArray(name);
            m_stack.push(m_current);
            m_current = arr;
        }

        void EndArray() override {
            m_current = m_stack.top();
            m_stack.pop();
        }

        void BeginArrayElement() override {
            m_current->push_back(nlohmann::json::object());
            m_stack.push(m_current);
            m_current = &m_current->back();
        }

        void EndArrayElement() override {
            m_current = m_stack.top();
            m_stack.pop();
        }

        void Write(std::string_view name, float value) override {
            (*m_current)[std::string(name)] = value;
        }

        void Write(std::string_view name, bool value) override {
            (*m_current)[std::string(name)] = value;
        }

        void Write(std::string_view name, int32_t value) override {
            (*m_current)[std::string(name)] = value;
        }

        void Write(std::string_view name, std::string_view value) override {
            (*m_current)[std::string(name)] = std::string(value);
        }

    private:
        nlohmann::json& m_root;
        nlohmann::json* m_current = nullptr;
        std::stack<nlohmann::json*> m_stack;

        nlohmann::json* CreateObject(std::string_view name) {
            if (name.empty()) {
                *m_current = nlohmann::json::object();
                return m_current;
            }

            auto key = std::string(name);
            (*m_current)[key] = nlohmann::json::object();
            return &(*m_current)[key];
        }

        nlohmann::json* CreateArray(std::string_view name) {
            if (name.empty()) {
                *m_current = nlohmann::json::array();
                return m_current;
            }

            auto key = std::string(name);
            (*m_current)[key] = nlohmann::json::array();
            return &(*m_current)[key];
        }
    };
}