#include "gnspch.h"
#include "Material.h"

#include <algorithm>
#include <cstring>
#include <utility>

namespace
{
    struct GpuLayoutInfo
    {
        size_t elementSize = 0;
        size_t alignment = 1;
        size_t elementStride = 0;
    };

    size_t AlignUp(size_t value, size_t alignment)
    {
        if (alignment <= 1)
        {
            return value;
        }

        return (value + alignment - 1) & ~(alignment - 1);
    }

    GpuLayoutInfo GetStd430LayoutInfo(gns::MaterialPropertyType type)
    {
        switch (type)
        {
        case gns::MaterialPropertyType::Float:
        case gns::MaterialPropertyType::Int:
        case gns::MaterialPropertyType::UInt:
        case gns::MaterialPropertyType::FloatArray:
        case gns::MaterialPropertyType::IntArray:
        case gns::MaterialPropertyType::UIntArray:
            return { 4, 4, 4 };
        case gns::MaterialPropertyType::Vec2:
            return { sizeof(glm::vec2), 8, sizeof(glm::vec2) };
        case gns::MaterialPropertyType::Vec3:
            return { sizeof(glm::vec3), 16, 16 };
        case gns::MaterialPropertyType::Vec4:
            return { sizeof(glm::vec4), 16, sizeof(glm::vec4) };
        case gns::MaterialPropertyType::Mat4:
            return { sizeof(glm::mat4), 16, sizeof(glm::mat4) };
        default:
            return {};
        }
    }

    size_t GetTotalSize(const GpuLayoutInfo& layoutInfo, uint32_t elementCount)
    {
        if (elementCount <= 1)
        {
            return layoutInfo.elementSize;
        }

        return layoutInfo.elementStride * elementCount;
    }

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
            property.elementCount != 1 ||
            property.elementSize != sizeof(T))
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

    void CopyPropertyData(
        const gns::MaterialPropertyInfo& sourceProperty,
        const std::vector<uint8_t>& sourceData,
        const gns::MaterialPropertyInfo& targetProperty,
        std::vector<uint8_t>& targetData)
    {
        if (sourceProperty.offset + sourceProperty.size > sourceData.size() ||
            targetProperty.offset + targetProperty.size > targetData.size())
        {
            return;
        }

        const uint32_t copyCount = std::min(sourceProperty.elementCount, targetProperty.elementCount);
        const size_t copySize = std::min(sourceProperty.elementSize, targetProperty.elementSize);
        for (uint32_t i = 0; i < copyCount; ++i)
        {
            const uint8_t* source =
                sourceData.data() + sourceProperty.offset + sourceProperty.elementStride * i;
            uint8_t* target =
                targetData.data() + targetProperty.offset + targetProperty.elementStride * i;
            std::memcpy(target, source, copySize);
        }
    }

    bool IsTypeCompatible(
        const gns::MaterialPropertyInfo& property,
        gns::MaterialPropertyType type,
        uint32_t elementCount)
    {
        return property.type == type && property.elementCount == elementCount;
    }
}

bool gns::MaterialLayout::AddProperty(
    const std::string& name,
    MaterialPropertyType type,
    uint32_t elementCount)
{
    const GpuLayoutInfo layoutInfo = GetStd430LayoutInfo(type);
    if (layoutInfo.elementSize == 0)
    {
        LOG_WARNING("[MaterialLayout]: Cannot add material property with unknown layout.");
        LOG_WARNING(name);
        return false;
    }

    const size_t offset = AlignUp(m_size, layoutInfo.alignment);
    const size_t size = GetTotalSize(layoutInfo, elementCount);
    return AddPropertyAtOffset(
        name,
        type,
        offset,
        size,
        elementCount,
        layoutInfo.elementStride,
        layoutInfo.alignment);
}

bool gns::MaterialLayout::AddPropertyAtOffset(
    const std::string& name,
    MaterialPropertyType type,
    size_t offset,
    size_t size,
    uint32_t elementCount,
    size_t elementStride,
    size_t alignment)
{
    if (name.empty() || size == 0 || elementCount == 0)
    {
        LOG_WARNING("[MaterialLayout]: Cannot add empty material property.");
        return false;
    }

    if (m_propertyIndices.contains(name))
    {
        LOG_WARNING("[MaterialLayout]: Duplicate material property ignored.");
        LOG_WARNING(name);
        return false;
    }

    const GpuLayoutInfo defaultLayout = GetStd430LayoutInfo(type);
    const size_t resolvedAlignment = alignment != 0 ? alignment : defaultLayout.alignment;
    const size_t resolvedElementSize = defaultLayout.elementSize != 0
        ? defaultLayout.elementSize
        : (elementCount > 1 ? size / elementCount : size);
    const size_t resolvedElementStride = elementStride != 0
        ? elementStride
        : (elementCount > 1 ? resolvedElementSize : resolvedElementSize);

    MaterialPropertyInfo property;
    property.name = name;
    property.type = type;
    property.offset = offset;
    property.size = size;
    property.elementSize = resolvedElementSize;
    property.elementStride = resolvedElementStride;
    property.alignment = resolvedAlignment != 0 ? resolvedAlignment : 1;
    property.elementCount = elementCount;

    m_propertyIndices[property.name] = m_properties.size();
    m_properties.emplace_back(std::move(property));
    m_size = std::max(m_size, offset + size);
    return true;
}

void gns::MaterialLayout::Clear()
{
    m_properties.clear();
    m_propertyIndices.clear();
    m_size = 0;
}

void gns::MaterialLayout::SetSize(size_t size)
{
    m_size = std::max(m_size, size);
}

bool gns::MaterialLayout::IsCompatibleWith(const MaterialLayout& other) const
{
    if (m_size != other.m_size || m_properties.size() != other.m_properties.size())
    {
        return false;
    }

    for (const MaterialPropertyInfo& property : m_properties)
    {
        const MaterialPropertyInfo* otherProperty = other.FindProperty(property.name);
        if (otherProperty == nullptr ||
            otherProperty->type != property.type ||
            otherProperty->offset != property.offset ||
            otherProperty->size != property.size ||
            otherProperty->elementSize != property.elementSize ||
            otherProperty->elementStride != property.elementStride ||
            otherProperty->alignment != property.alignment ||
            otherProperty->elementCount != property.elementCount)
        {
            return false;
        }
    }

    return true;
}

const gns::MaterialPropertyInfo* gns::MaterialLayout::FindProperty(const std::string& name) const
{
    const auto property = m_propertyIndices.find(name);
    if (property == m_propertyIndices.end())
    {
        return nullptr;
    }

    return &m_properties[property->second];
}

void gns::MaterialLayout::RebuildLookup()
{
    m_propertyIndices.clear();
    for (size_t i = 0; i < m_properties.size(); ++i)
    {
        m_propertyIndices[m_properties[i].name] = i;
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

void gns::Material::SetLayout(const MaterialLayout& layout, bool preserveValues)
{
    if (m_layout.IsCompatibleWith(layout))
    {
        return;
    }

    const MaterialLayout oldLayout = m_layout;
    const std::vector<uint8_t> oldData = m_dataBlob;

    m_layout = layout;
    m_dataBlob.assign(m_layout.GetSize(), 0);

    if (!preserveValues)
    {
        return;
    }

    for (const MaterialPropertyInfo& targetProperty : m_layout.GetProperties())
    {
        const MaterialPropertyInfo* sourceProperty = oldLayout.FindProperty(targetProperty.name);
        if (sourceProperty == nullptr || sourceProperty->type != targetProperty.type)
        {
            continue;
        }

        CopyPropertyData(*sourceProperty, oldData, targetProperty, m_dataBlob);
    }
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
    if (name.empty() || data == nullptr || size == 0 || elementCount == 0)
    {
        LOG_WARNING("[Material]: Cannot set empty material property.");
        return;
    }

    if (!EnsureProperty(name, type, elementCount))
    {
        return;
    }

    MaterialPropertyInfo property;
    if (!TryGetProperty(name, property))
    {
        return;
    }

    uint8_t* destination = static_cast<uint8_t*>(GetMutablePropertyData(property));
    if (destination == nullptr)
    {
        return;
    }

    std::memset(destination, 0, property.size);

    const uint8_t* source = static_cast<const uint8_t*>(data);
    if (property.elementCount > 1)
    {
        const size_t sourceElementSize = size / elementCount;
        const uint32_t copyCount = std::min(elementCount, property.elementCount);
        const size_t copySize = std::min(sourceElementSize, property.elementSize);
        for (uint32_t i = 0; i < copyCount; ++i)
        {
            std::memcpy(
                destination + property.elementStride * i,
                source + sourceElementSize * i,
                copySize);
        }
        return;
    }

    std::memcpy(destination, source, std::min(size, property.size));
}

bool gns::Material::TryGetProperty(const std::string& name, MaterialPropertyInfo& outInfo) const
{
    const MaterialPropertyInfo* property = m_layout.FindProperty(name);
    if (property == nullptr)
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

float gns::Material::GetFloat(const std::string& name, float fallback) const
{
    float value = fallback;
    return TryGetFloat(name, value) ? value : fallback;
}

int32_t gns::Material::GetInt(const std::string& name, int32_t fallback) const
{
    int32_t value = fallback;
    return TryGetInt(name, value) ? value : fallback;
}

uint32_t gns::Material::GetUInt(const std::string& name, uint32_t fallback) const
{
    uint32_t value = fallback;
    return TryGetUInt(name, value) ? value : fallback;
}

glm::vec2 gns::Material::GetVec2(const std::string& name, const glm::vec2& fallback) const
{
    glm::vec2 value = fallback;
    return TryGetVec2(name, value) ? value : fallback;
}

glm::vec3 gns::Material::GetVec3(const std::string& name, const glm::vec3& fallback) const
{
    glm::vec3 value = fallback;
    return TryGetVec3(name, value) ? value : fallback;
}

glm::vec4 gns::Material::GetVec4(const std::string& name, const glm::vec4& fallback) const
{
    glm::vec4 value = fallback;
    return TryGetVec4(name, value) ? value : fallback;
}

glm::vec3 gns::Material::GetColor3(const std::string& name, const glm::vec3& fallback) const
{
    return GetVec3(name, fallback);
}

glm::vec4 gns::Material::GetColor4(const std::string& name, const glm::vec4& fallback) const
{
    return GetVec4(name, fallback);
}

glm::mat4 gns::Material::GetMat4(const std::string& name, const glm::mat4& fallback) const
{
    glm::mat4 value = fallback;
    return TryGetMat4(name, value) ? value : fallback;
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
    m_layout.Clear();
    m_dataBlob.clear();
}

bool gns::Material::EnsureProperty(
    const std::string& name,
    MaterialPropertyType type,
    uint32_t elementCount)
{
    MaterialPropertyInfo property;
    if (TryGetProperty(name, property))
    {
        if (!IsTypeCompatible(property, type, elementCount))
        {
            LOG_WARNING("[Material]: Material property type or count does not match layout.");
            LOG_WARNING(name);
            return false;
        }

        return true;
    }

    if (!m_layout.AddProperty(name, type, elementCount))
    {
        return false;
    }

    m_dataBlob.resize(m_layout.GetSize(), 0);
    return true;
}

void* gns::Material::GetMutablePropertyData(const MaterialPropertyInfo& property)
{
    if (property.offset + property.size > m_dataBlob.size())
    {
        return nullptr;
    }

    return m_dataBlob.data() + property.offset;
}
