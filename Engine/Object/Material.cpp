#include "gnspch.h"
#include "Material.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

namespace
{
    constexpr std::array<const char*, 4> ImportedBaseColorTextureNames =
    {
        "albedoTexture",
        "albedo_texture",
        "baseColorTexture",
        "base_color_texture"
    };

    constexpr std::array<const char*, 4> ImportedBaseColorNames =
    {
        "albedo_color",
        "baseColor",
        "base_color",
        "baseColorFactor"
    };

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
        case gns::MaterialPropertyType::Color3:
            return { sizeof(glm::vec3), 16, 16 };
        case gns::MaterialPropertyType::Vec4:
        case gns::MaterialPropertyType::Color4:
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

    bool IsTypeCompatible(
        const gns::MaterialPropertyInfo& property,
        gns::MaterialPropertyType type,
        uint32_t elementCount);

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
            !IsTypeCompatible(property, type, 1) ||
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
        if (!sourceProperty.IsBufferBacked() || !targetProperty.IsBufferBacked())
        {
            return;
        }

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
        const bool exactType = property.type == type;
        const bool compatibleVecColor =
            (property.type == gns::MaterialPropertyType::Color3 && type == gns::MaterialPropertyType::Vec3) ||
            (property.type == gns::MaterialPropertyType::Vec3 && type == gns::MaterialPropertyType::Color3) ||
            (property.type == gns::MaterialPropertyType::Color4 && type == gns::MaterialPropertyType::Vec4) ||
            (property.type == gns::MaterialPropertyType::Vec4 && type == gns::MaterialPropertyType::Color4);

        return (exactType || compatibleVecColor) && property.elementCount == elementCount;
    }

    template<size_t Count>
    bool TryFindMaterialProperty(
        const gns::Material& material,
        const std::array<const char*, Count>& names,
        gns::MaterialPropertyType type,
        gns::MaterialPropertyInfo& outProperty)
    {
        for (const char* name : names)
        {
            gns::MaterialPropertyInfo property;
            if (material.TryGetProperty(name, property) && property.type == type)
            {
                outProperty = property;
                return true;
            }
        }

        return false;
    }
}

bool gns::MaterialLayout::AddProperty(
    const std::string& name,
    MaterialPropertyType type,
    uint32_t elementCount,
    uint32_t set,
    uint32_t binding,
    MaterialDescriptorKind descriptorKind)
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
        layoutInfo.alignment,
        set,
        binding,
        1,
        descriptorKind);
}

bool gns::MaterialLayout::AddPropertyAtOffset(
    const std::string& name,
    MaterialPropertyType type,
    size_t offset,
    size_t size,
    uint32_t elementCount,
    size_t elementStride,
    size_t alignment,
    uint32_t set,
    uint32_t binding,
    uint32_t descriptorCount,
    MaterialDescriptorKind descriptorKind)
{
    if (name.empty() || elementCount == 0 || descriptorCount == 0 ||
        (type != MaterialPropertyType::Texture2D && size == 0))
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
    property.set = set;
    property.binding = binding;
    property.descriptorCount = descriptorCount;
    property.descriptorKind = descriptorKind;

    m_propertyIndices[property.name] = m_properties.size();
    m_properties.emplace_back(std::move(property));
    m_size = std::max(m_size, offset + size);
    return true;
}

bool gns::MaterialLayout::AddTextureProperty(
    const std::string& name,
    uint32_t set,
    uint32_t binding,
    uint32_t descriptorCount)
{
    return AddPropertyAtOffset(
        name,
        MaterialPropertyType::Texture2D,
        0,
        0,
        1,
        0,
        1,
        set,
        binding,
        descriptorCount,
        MaterialDescriptorKind::Texture);
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
            otherProperty->elementCount != property.elementCount ||
            otherProperty->set != property.set ||
            otherProperty->binding != property.binding ||
            otherProperty->descriptorCount != property.descriptorCount ||
            otherProperty->descriptorKind != property.descriptorKind)
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
}

gns::Material::Material(std::string name)
    : Object(std::move(name)),
      albedo_texture(Handle::CreateFromString(DefaultResourceNames::WhiteTexture))
{
}

gns::Material::Material(Handle handle, std::string name)
    : Object(handle, std::move(name)),
      albedo_texture(Handle::CreateFromString(DefaultResourceNames::WhiteTexture))
{
}

void gns::Material::SetLayout(const MaterialLayout& layout, bool preserveValues)
{
    if (m_layout.IsCompatibleWith(layout))
    {
        const size_t texturePropertyCount = std::count_if(
            m_layout.GetProperties().begin(),
            m_layout.GetProperties().end(),
            [](const MaterialPropertyInfo& property)
            {
                return property.type == MaterialPropertyType::Texture2D;
            });
        if (m_textureSlots.size() != texturePropertyCount ||
            m_texturePropertyIndices.size() != texturePropertyCount ||
            m_textureNameToSlot.size() != texturePropertyCount)
        {
            RebuildTextureSlots();
        }
        return;
    }

    const MaterialLayout oldLayout = m_layout;
    const std::vector<uint8_t> oldData = m_dataBlob;
    std::unordered_map<std::string, Reference<gns::Texture>> oldTextures;
    for (size_t slotIndex = 0; slotIndex < m_textureSlots.size(); ++slotIndex)
    {
        if (slotIndex >= m_texturePropertyIndices.size())
        {
            continue;
        }

        const size_t propertyIndex = m_texturePropertyIndices[slotIndex];
        const std::vector<MaterialPropertyInfo>& oldProperties = oldLayout.GetProperties();
        if (propertyIndex < oldProperties.size())
        {
            oldTextures[oldProperties[propertyIndex].name] = m_textureSlots[slotIndex];
        }
    }

    m_layout = layout;
    m_dataBlob.assign(m_layout.GetSize(), 0);
    RebuildTextureSlots(preserveValues ? &oldTextures : nullptr);

    if (!preserveValues)
    {
        return;
    }

    for (const MaterialPropertyInfo& targetProperty : m_layout.GetProperties())
    {
        const MaterialPropertyInfo* sourceProperty = oldLayout.FindProperty(targetProperty.name);
        if (sourceProperty == nullptr ||
            !IsTypeCompatible(*sourceProperty, targetProperty.type, targetProperty.elementCount))
        {
            continue;
        }

        CopyPropertyData(*sourceProperty, oldData, targetProperty, m_dataBlob);
    }
}

const gns::MaterialLayout& gns::Material::GetLayout() const
{
    return m_layout;
}

void gns::Material::ApplyImportCompatibilityDefaults()
{
    MaterialPropertyInfo property;
    if (albedo_texture.m_handle.IsValid() &&
        TryFindMaterialProperty(
            *this,
            ImportedBaseColorTextureNames,
            MaterialPropertyType::Texture2D,
            property))
    {
        Reference<gns::Texture> currentTexture;
        if (!TryGetTexture(property.name, currentTexture) ||
            !currentTexture.m_handle.IsValid())
        {
            SetTexture(property.name, albedo_texture);
        }
    }

    if (TryFindMaterialProperty(
        *this,
        ImportedBaseColorNames,
        MaterialPropertyType::Color4,
        property))
    {
        SetColor4(property.name, albedo_color);
        return;
    }

    if (TryFindMaterialProperty(
        *this,
        ImportedBaseColorNames,
        MaterialPropertyType::Vec4,
        property))
    {
        SetVec4(property.name, albedo_color);
        return;
    }

    if (TryFindMaterialProperty(
        *this,
        ImportedBaseColorNames,
        MaterialPropertyType::Color3,
        property))
    {
        SetColor3(property.name, glm::vec3(albedo_color));
        return;
    }

    if (TryFindMaterialProperty(
        *this,
        ImportedBaseColorNames,
        MaterialPropertyType::Vec3,
        property))
    {
        SetVec3(property.name, glm::vec3(albedo_color));
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
    SetValue(*this, name, MaterialPropertyType::Color3, value);
}

void gns::Material::SetColor4(const std::string& name, const glm::vec4& value)
{
    SetValue(*this, name, MaterialPropertyType::Color4, value);
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

void gns::Material::SetTexture(const std::string& name, Reference<gns::Texture> texture)
{
    MaterialPropertyInfo property;
    if (!TryGetProperty(name, property) || property.type != MaterialPropertyType::Texture2D)
    {
        LOG_WARNING("[Material]: Cannot set texture because the property is not in the material layout.");
        LOG_WARNING(name);
        return;
    }

    const auto slot = m_textureNameToSlot.find(name);
    if (slot == m_textureNameToSlot.end() || slot->second >= m_textureSlots.size())
    {
        LOG_WARNING("[Material]: Cannot set texture because the texture slot is missing.");
        LOG_WARNING(name);
        return;
    }

    m_textureSlots[slot->second] = texture;
}

void gns::Material::SetTexture(const std::string& name, Handle textureHandle)
{
    SetTexture(name, Reference<gns::Texture>(textureHandle));
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

    if (!property.IsBufferBacked())
    {
        LOG_WARNING("[Material]: Cannot write bytes to a non-buffer material property.");
        LOG_WARNING(name);
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
    return TryGetValue(*this, name, MaterialPropertyType::Color3, outValue);
}

bool gns::Material::TryGetColor4(const std::string& name, glm::vec4& outValue) const
{
    return TryGetValue(*this, name, MaterialPropertyType::Color4, outValue);
}

bool gns::Material::TryGetMat4(const std::string& name, glm::mat4& outValue) const
{
    return TryGetValue(*this, name, MaterialPropertyType::Mat4, outValue);
}

bool gns::Material::TryGetTexture(const std::string& name, Reference<gns::Texture>& outTexture) const
{
    MaterialPropertyInfo property;
    if (!TryGetProperty(name, property) || property.type != MaterialPropertyType::Texture2D)
    {
        return false;
    }

    const auto slot = m_textureNameToSlot.find(name);
    if (slot == m_textureNameToSlot.end())
    {
        return false;
    }

    return TryGetTexture(slot->second, outTexture);
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
    glm::vec3 value = fallback;
    return TryGetColor3(name, value) ? value : fallback;
}

glm::vec4 gns::Material::GetColor4(const std::string& name, const glm::vec4& fallback) const
{
    glm::vec4 value = fallback;
    return TryGetColor4(name, value) ? value : fallback;
}

glm::mat4 gns::Material::GetMat4(const std::string& name, const glm::mat4& fallback) const
{
    glm::mat4 value = fallback;
    return TryGetMat4(name, value) ? value : fallback;
}

gns::Reference<gns::Texture> gns::Material::GetTexture(
    const std::string& name,
    Reference<gns::Texture> fallback) const
{
    Reference<gns::Texture> value;
    return TryGetTexture(name, value) ? value : fallback;
}

gns::Handle gns::Material::GetTextureHandle(const std::string& name, Handle fallback) const
{
    Reference<gns::Texture> value;
    return TryGetTexture(name, value) ? value.m_handle : fallback;
}

size_t gns::Material::GetTextureSlotCount() const
{
    return m_textureSlots.size();
}

const gns::MaterialPropertyInfo* gns::Material::GetTextureSlotProperty(size_t slotIndex) const
{
    if (slotIndex >= m_texturePropertyIndices.size())
    {
        return nullptr;
    }

    const size_t propertyIndex = m_texturePropertyIndices[slotIndex];
    const std::vector<MaterialPropertyInfo>& properties = m_layout.GetProperties();
    return propertyIndex < properties.size() ? &properties[propertyIndex] : nullptr;
}

bool gns::Material::TryGetTexture(size_t slotIndex, Reference<gns::Texture>& outTexture) const
{
    if (slotIndex >= m_textureSlots.size())
    {
        return false;
    }

    outTexture = m_textureSlots[slotIndex];
    return outTexture.m_handle.IsValid();
}

gns::Handle gns::Material::GetTextureHandle(size_t slotIndex, Handle fallback) const
{
    Reference<gns::Texture> value;
    return TryGetTexture(slotIndex, value) ? value.m_handle : fallback;
}

float* gns::Material::GetFloatPtr(const std::string& name)
{
    return GetValuePtr<float>(name);
}

int32_t* gns::Material::GetIntPtr(const std::string& name)
{
    return GetValuePtr<int32_t>(name);
}

uint32_t* gns::Material::GetUIntPtr(const std::string& name)
{
    return GetValuePtr<uint32_t>(name);
}

glm::vec2* gns::Material::GetVec2Ptr(const std::string& name)
{
    return GetValuePtr<glm::vec2>(name);
}

glm::vec3* gns::Material::GetVec3Ptr(const std::string& name)
{
    return GetValuePtr<glm::vec3>(name);
}

glm::vec4* gns::Material::GetVec4Ptr(const std::string& name)
{
    return GetValuePtr<glm::vec4>(name);
}

glm::vec3* gns::Material::GetColor3Ptr(const std::string& name)
{
    MaterialPropertyInfo property;
    if (!TryGetProperty(name, property) ||
        !IsTypeCompatible(property, MaterialPropertyType::Color3, 1) ||
        property.elementSize != sizeof(glm::vec3))
    {
        return nullptr;
    }

    return static_cast<glm::vec3*>(GetMutablePropertyData(property));
}

glm::vec4* gns::Material::GetColor4Ptr(const std::string& name)
{
    MaterialPropertyInfo property;
    if (!TryGetProperty(name, property) ||
        !IsTypeCompatible(property, MaterialPropertyType::Color4, 1) ||
        property.elementSize != sizeof(glm::vec4))
    {
        return nullptr;
    }

    return static_cast<glm::vec4*>(GetMutablePropertyData(property));
}

glm::mat4* gns::Material::GetMat4Ptr(const std::string& name)
{
    return GetValuePtr<glm::mat4>(name);
}

const float* gns::Material::GetFloatPtr(const std::string& name) const
{
    return GetValuePtr<float>(name);
}

const int32_t* gns::Material::GetIntPtr(const std::string& name) const
{
    return GetValuePtr<int32_t>(name);
}

const uint32_t* gns::Material::GetUIntPtr(const std::string& name) const
{
    return GetValuePtr<uint32_t>(name);
}

const glm::vec2* gns::Material::GetVec2Ptr(const std::string& name) const
{
    return GetValuePtr<glm::vec2>(name);
}

const glm::vec3* gns::Material::GetVec3Ptr(const std::string& name) const
{
    return GetValuePtr<glm::vec3>(name);
}

const glm::vec4* gns::Material::GetVec4Ptr(const std::string& name) const
{
    return GetValuePtr<glm::vec4>(name);
}

const glm::vec3* gns::Material::GetColor3Ptr(const std::string& name) const
{
    MaterialPropertyInfo property;
    if (!TryGetProperty(name, property) ||
        !IsTypeCompatible(property, MaterialPropertyType::Color3, 1) ||
        property.elementSize != sizeof(glm::vec3))
    {
        return nullptr;
    }

    return static_cast<const glm::vec3*>(GetPropertyData(property));
}

const glm::vec4* gns::Material::GetColor4Ptr(const std::string& name) const
{
    MaterialPropertyInfo property;
    if (!TryGetProperty(name, property) ||
        !IsTypeCompatible(property, MaterialPropertyType::Color4, 1) ||
        property.elementSize != sizeof(glm::vec4))
    {
        return nullptr;
    }

    return static_cast<const glm::vec4*>(GetPropertyData(property));
}

const glm::mat4* gns::Material::GetMat4Ptr(const std::string& name) const
{
    return GetValuePtr<glm::mat4>(name);
}

const std::vector<gns::MaterialPropertyInfo>& gns::Material::GetProperties() const
{
    return m_layout.GetProperties();
}

gns::MaterialDataBlob gns::Material::GetDataBlob() const
{
    return { m_dataBlob.data(), m_dataBlob.size() };
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
    m_textureSlots.clear();
    m_texturePropertyIndices.clear();
    m_textureNameToSlot.clear();
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

    LOG_WARNING("[Material]: Cannot create material property without reflected layout metadata.");
    LOG_WARNING(name);
    return false;
}

void* gns::Material::GetMutablePropertyData(const MaterialPropertyInfo& property)
{
    if (property.offset + property.size > m_dataBlob.size())
    {
        return nullptr;
    }

    return m_dataBlob.data() + property.offset;
}

void gns::Material::RebuildTextureSlots(
    const std::unordered_map<std::string, Reference<gns::Texture>>* preservedTextures)
{
    m_textureSlots.clear();
    m_texturePropertyIndices.clear();
    m_textureNameToSlot.clear();

    const std::vector<MaterialPropertyInfo>& properties = m_layout.GetProperties();
    for (size_t propertyIndex = 0; propertyIndex < properties.size(); ++propertyIndex)
    {
        const MaterialPropertyInfo& property = properties[propertyIndex];
        if (property.type != MaterialPropertyType::Texture2D)
        {
            continue;
        }

        const size_t slotIndex = m_textureSlots.size();
        Reference<gns::Texture> texture;
        if (preservedTextures != nullptr)
        {
            if (const auto preserved = preservedTextures->find(property.name);
                preserved != preservedTextures->end())
            {
                texture = preserved->second;
            }
        }

        m_textureSlots.emplace_back(texture);
        m_texturePropertyIndices.emplace_back(propertyIndex);
        m_textureNameToSlot[property.name] = slotIndex;
    }
}
