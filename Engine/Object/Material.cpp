#include "gnspch.h"
#include "Material.h"

#include <cstring>
#include <utility>

namespace
{
    template<typename T>
    void SetValue(gns::Material& material, const std::string& name, gns::MaterialPropertyType type, const T& value)
    {
        material.SetBytes(name, type, &value, sizeof(T));
    }

    template<typename T>
    bool TryGetValue(
        const gns::Material& material,
        const std::string& name,
        gns::MaterialPropertyType type,
        T& outValue)
    {
        gns::MaterialPropertyInfo property;
        if (!material.TryGetProperty(name, property) ||
            property.type != type ||
            property.size != sizeof(T))
        {
            return false;
        }

        const void* data = material.GetPropertyData(property);
        if (data == nullptr)
        {
            return false;
        }

        std::memcpy(&outValue, data, sizeof(T));
        return true;
    }

    template<typename T>
    void SetArray(
        gns::Material& material,
        const std::string& name,
        gns::MaterialPropertyType type,
        std::span<const T> values)
    {
        material.SetBytes(
            name,
            type,
            values.data(),
            values.size_bytes(),
            static_cast<uint32_t>(values.size()));
    }
}

gns::Material::Material()
    : Object(Handle::New(), "Material"),
      albedo_texture(Handle::CreateFromString(DefaultResourceNames::WhiteTexture))
{
    SetVec4("albedo_color", albedo_color);
}

gns::Material::Material(std::string name)
    : Object(std::move(name)),
      albedo_texture(Handle::CreateFromString(DefaultResourceNames::WhiteTexture))
{
    SetVec4("albedo_color", albedo_color);
}

gns::Material::Material(Handle handle, std::string name)
    : Object(handle, std::move(name)),
      albedo_texture(Handle::CreateFromString(DefaultResourceNames::WhiteTexture))
{
    SetVec4("albedo_color", albedo_color);
}

void gns::Material::SetFloat(const std::string& name, float value)
{
    SetValue(*this, name, MaterialPropertyType::Float, value);
}

void gns::Material::SetInt(const std::string& name, int32_t value)
{
    SetValue(*this, name, MaterialPropertyType::Int, value);
}

void gns::Material::SetUInt(const std::string& name, uint32_t value)
{
    SetValue(*this, name, MaterialPropertyType::UInt, value);
}

void gns::Material::SetVec2(const std::string& name, const glm::vec2& value)
{
    SetValue(*this, name, MaterialPropertyType::Vec2, value);
}

void gns::Material::SetVec3(const std::string& name, const glm::vec3& value)
{
    SetValue(*this, name, MaterialPropertyType::Vec3, value);
}

void gns::Material::SetVec4(const std::string& name, const glm::vec4& value)
{
    SetValue(*this, name, MaterialPropertyType::Vec4, value);
}

void gns::Material::SetColor3(const std::string& name, const glm::vec3& value)
{
    SetVec3(name, value);
}

void gns::Material::SetColor4(const std::string& name, const glm::vec4& value)
{
    SetVec4(name, value);
}

void gns::Material::SetMat4(const std::string& name, const glm::mat4& value)
{
    SetValue(*this, name, MaterialPropertyType::Mat4, value);
}

void gns::Material::SetFloatArray(const std::string& name, std::span<const float> values)
{
    SetArray(*this, name, MaterialPropertyType::FloatArray, values);
}

void gns::Material::SetIntArray(const std::string& name, std::span<const int32_t> values)
{
    SetArray(*this, name, MaterialPropertyType::IntArray, values);
}

void gns::Material::SetUIntArray(const std::string& name, std::span<const uint32_t> values)
{
    SetArray(*this, name, MaterialPropertyType::UIntArray, values);
}

void gns::Material::SetBytes(
    const std::string& name,
    MaterialPropertyType type,
    const void* data,
    size_t size,
    uint32_t elementCount)
{
    if (name.empty() || data == nullptr || size == 0)
    {
        LOG_WARNING("[Material]: Cannot set empty material property.");
        return;
    }

    auto existingProperty = std::find_if(
        m_properties.begin(),
        m_properties.end(),
        [&](const MaterialPropertyInfo& property)
        {
            return property.name == name;
        });

    if (existingProperty != m_properties.end() && existingProperty->size == size)
    {
        existingProperty->type = type;
        existingProperty->elementCount = elementCount;
        std::memcpy(m_dataBlob.data() + existingProperty->offset, data, size);
        return;
    }

    if (existingProperty != m_properties.end())
    {
        m_properties.erase(existingProperty);
        std::vector<uint8_t> rebuiltBlob;
        rebuiltBlob.reserve(m_dataBlob.size() + size);

        size_t nextOffset = 0;
        for (MaterialPropertyInfo& property : m_properties)
        {
            const uint8_t* source = m_dataBlob.data() + property.offset;
            rebuiltBlob.insert(rebuiltBlob.end(), source, source + property.size);
            property.offset = nextOffset;
            nextOffset += property.size;
        }

        m_dataBlob = std::move(rebuiltBlob);
    }

    MaterialPropertyInfo property;
    property.name = name;
    property.type = type;
    property.offset = m_dataBlob.size();
    property.size = size;
    property.elementCount = elementCount;

    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    m_dataBlob.insert(m_dataBlob.end(), bytes, bytes + size);
    m_properties.emplace_back(std::move(property));
}

bool gns::Material::TryGetProperty(const std::string& name, MaterialPropertyInfo& outInfo) const
{
    const auto property = std::find_if(
        m_properties.begin(),
        m_properties.end(),
        [&](const MaterialPropertyInfo& candidate)
        {
            return candidate.name == name;
        });

    if (property == m_properties.end())
    {
        return false;
    }

    outInfo = *property;
    return true;
}

bool gns::Material::TryGetFloat(const std::string& name, float& outValue) const
{
    return TryGetValue(*this, name, MaterialPropertyType::Float, outValue);
}

bool gns::Material::TryGetInt(const std::string& name, int32_t& outValue) const
{
    return TryGetValue(*this, name, MaterialPropertyType::Int, outValue);
}

bool gns::Material::TryGetUInt(const std::string& name, uint32_t& outValue) const
{
    return TryGetValue(*this, name, MaterialPropertyType::UInt, outValue);
}

bool gns::Material::TryGetVec2(const std::string& name, glm::vec2& outValue) const
{
    return TryGetValue(*this, name, MaterialPropertyType::Vec2, outValue);
}

bool gns::Material::TryGetVec3(const std::string& name, glm::vec3& outValue) const
{
    return TryGetValue(*this, name, MaterialPropertyType::Vec3, outValue);
}

bool gns::Material::TryGetVec4(const std::string& name, glm::vec4& outValue) const
{
    return TryGetValue(*this, name, MaterialPropertyType::Vec4, outValue);
}

bool gns::Material::TryGetColor3(const std::string& name, glm::vec3& outValue) const
{
    return TryGetVec3(name, outValue);
}

bool gns::Material::TryGetColor4(const std::string& name, glm::vec4& outValue) const
{
    return TryGetVec4(name, outValue);
}

bool gns::Material::TryGetMat4(const std::string& name, glm::mat4& outValue) const
{
    return TryGetValue(*this, name, MaterialPropertyType::Mat4, outValue);
}

const void* gns::Material::GetPropertyData(const MaterialPropertyInfo& property) const
{
    if (property.offset + property.size > m_dataBlob.size())
    {
        return nullptr;
    }

    return m_dataBlob.data() + property.offset;
}

void gns::Material::ClearProperties()
{
    m_properties.clear();
    m_dataBlob.clear();
}
