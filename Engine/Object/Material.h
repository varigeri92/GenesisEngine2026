#pragma once
#include <optional>
#include <variant>
#include <glm/glm.hpp>
#include "IObject.h"

namespace gns
{
    struct Shader;
    template <typename T>
    concept MaterialPropertyType =
           std::same_as<T, uint32_t>
        || std::same_as<T, float>
        || std::same_as<T, glm::vec2>
        || std::same_as<T, glm::vec3>
        || std::same_as<T, glm::vec4>
        || std::same_as<T, Handle>;
        
    struct MaterialProperty
    {
        std::string name;
        std::variant<uint32_t, float, glm::vec2, glm::vec3, glm::vec4, Handle> value;
    };
    struct Material : public Object
    {
        std::unordered_map<std::string, MaterialProperty> properties;
        Reference<gns::Shader> shader_ref;
        
        template <typename T>
        T Get(const std::string& name)
        {
            if (properties.contains(name))
            {
                if (auto v = std::get_if<T>(properties.at(name).value))
                {
                    return *v;
                }
                LOG_ERROR("CantGet Material property, Type mismatch return 0");
            }
            return std::optional<T>();
        }
        
        template <MaterialPropertyType T>
        void SetProperty(const std::string& name, const T value)
        {
            if (properties.contains(name))
                properties[name] = value;
            else
                LOG_ERROR("Property does not exists. Use add property instead");
        }
        
        template <MaterialPropertyType T>
        void AddProperty(const std::string& name, const T value)
        {
            properties[name] = MaterialProperty{name, value};
        }
        
    };
}
