#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "IObject.h"
#include "Texture.h"

namespace gns
{
    struct Shader;

    inline constexpr uint32_t InvalidMaterialBinding = UINT32_MAX;

    enum class MaterialPropertyType : uint8_t
    {
        Unknown,
        Float,
        Int,
        UInt,
        Vec2,
        Vec3,
        Vec4,
        Color3,
        Color4,
        Mat4,
        FloatArray,
        IntArray,
        UIntArray,
        Texture2D,
        Bytes
    };

    enum class MaterialDescriptorKind : uint8_t
    {
        None,
        UniformBuffer,
        StorageBuffer,
        Texture
    };

    struct MaterialPropertyInfo
    {
        std::string name;
        MaterialPropertyType type = MaterialPropertyType::Unknown;
        size_t offset = 0;
        size_t size = 0;
        size_t elementSize = 0;
        size_t elementStride = 0;
        size_t alignment = 1;
        uint32_t elementCount = 1;
        uint32_t set = InvalidMaterialBinding;
        uint32_t binding = InvalidMaterialBinding;
        uint32_t descriptorCount = 1;
        MaterialDescriptorKind descriptorKind = MaterialDescriptorKind::None;

        bool IsArray() const { return elementCount > 1; }
        bool IsTexture() const { return type == MaterialPropertyType::Texture2D; }
        bool IsBufferBacked() const { return !IsTexture(); }
    };

    struct MaterialDataBlob
    {
        const void* data = nullptr;
        size_t size = 0;

        bool IsValid() const { return data != nullptr && size > 0; }
    };

    struct MaterialLayout
    {
        GNS_API bool AddProperty(
            const std::string& name,
            MaterialPropertyType type,
            uint32_t elementCount = 1,
            uint32_t set = InvalidMaterialBinding,
            uint32_t binding = InvalidMaterialBinding,
            MaterialDescriptorKind descriptorKind = MaterialDescriptorKind::None);
        GNS_API bool AddPropertyAtOffset(
            const std::string& name,
            MaterialPropertyType type,
            size_t offset,
            size_t size,
            uint32_t elementCount = 1,
            size_t elementStride = 0,
            size_t alignment = 0,
            uint32_t set = InvalidMaterialBinding,
            uint32_t binding = InvalidMaterialBinding,
            uint32_t descriptorCount = 1,
            MaterialDescriptorKind descriptorKind = MaterialDescriptorKind::None);
        GNS_API bool AddTextureProperty(
            const std::string& name,
            uint32_t set,
            uint32_t binding,
            uint32_t descriptorCount = 1);

        GNS_API void Clear();
        GNS_API void SetSize(size_t size);
        bool IsValid() const { return m_size > 0 || !m_properties.empty(); }
        GNS_API bool IsCompatibleWith(const MaterialLayout& other) const;
        size_t GetSize() const { return m_size; }
        const std::vector<MaterialPropertyInfo>& GetProperties() const { return m_properties; }
        GNS_API const MaterialPropertyInfo* FindProperty(const std::string& name) const;

    private:
        std::vector<MaterialPropertyInfo> m_properties;
        std::unordered_map<std::string, size_t> m_propertyIndices;
        size_t m_size = 0;

        void RebuildLookup();
    };

    template<typename T>
    struct MaterialValueTypeResolver
    {
        static constexpr bool Supported = false;
        static constexpr MaterialPropertyType Type = MaterialPropertyType::Unknown;
    };

    template<> struct MaterialValueTypeResolver<float>
    {
        static constexpr bool Supported = true;
        static constexpr MaterialPropertyType Type = MaterialPropertyType::Float;
    };

    template<> struct MaterialValueTypeResolver<int32_t>
    {
        static constexpr bool Supported = true;
        static constexpr MaterialPropertyType Type = MaterialPropertyType::Int;
    };

    template<> struct MaterialValueTypeResolver<uint32_t>
    {
        static constexpr bool Supported = true;
        static constexpr MaterialPropertyType Type = MaterialPropertyType::UInt;
    };

    template<> struct MaterialValueTypeResolver<glm::vec2>
    {
        static constexpr bool Supported = true;
        static constexpr MaterialPropertyType Type = MaterialPropertyType::Vec2;
    };

    template<> struct MaterialValueTypeResolver<glm::vec3>
    {
        static constexpr bool Supported = true;
        static constexpr MaterialPropertyType Type = MaterialPropertyType::Vec3;
    };

    template<> struct MaterialValueTypeResolver<glm::vec4>
    {
        static constexpr bool Supported = true;
        static constexpr MaterialPropertyType Type = MaterialPropertyType::Vec4;
    };

    template<> struct MaterialValueTypeResolver<glm::mat4>
    {
        static constexpr bool Supported = true;
        static constexpr MaterialPropertyType Type = MaterialPropertyType::Mat4;
    };

    struct Material : public Object
    {
        Reference<gns::Shader> shader_ref;

        GNS_API Material();
        GNS_API explicit Material(std::string name);
        GNS_API Material(Handle handle, std::string name);

        GNS_API void SetLayout(const MaterialLayout& layout, bool preserveValues = true);
        GNS_API const MaterialLayout& GetLayout() const;

        GNS_API void SetFloat(const std::string& name, float value);
        GNS_API void SetInt(const std::string& name, int32_t value);
        GNS_API void SetUInt(const std::string& name, uint32_t value);
        GNS_API void SetVec2(const std::string& name, const glm::vec2& value);
        GNS_API void SetVec3(const std::string& name, const glm::vec3& value);
        GNS_API void SetVec4(const std::string& name, const glm::vec4& value);
        GNS_API void SetColor3(const std::string& name, const glm::vec3& value);
        GNS_API void SetColor4(const std::string& name, const glm::vec4& value);
        GNS_API void SetMat4(const std::string& name, const glm::mat4& value);
        GNS_API void SetFloatArray(const std::string& name, std::span<const float> values);
        GNS_API void SetIntArray(const std::string& name, std::span<const int32_t> values);
        GNS_API void SetUIntArray(const std::string& name, std::span<const uint32_t> values);
        GNS_API void SetTexture(const std::string& name, Reference<gns::Texture> texture);
        GNS_API void SetTexture(const std::string& name, Handle textureHandle);
        GNS_API void SetBytes(
            const std::string& name,
            MaterialPropertyType type,
            const void* data,
            size_t size,
            uint32_t elementCount = 1);

        GNS_API bool TryGetProperty(const std::string& name, MaterialPropertyInfo& outInfo) const;
        GNS_API bool TryGetFloat(const std::string& name, float& outValue) const;
        GNS_API bool TryGetInt(const std::string& name, int32_t& outValue) const;
        GNS_API bool TryGetUInt(const std::string& name, uint32_t& outValue) const;
        GNS_API bool TryGetVec2(const std::string& name, glm::vec2& outValue) const;
        GNS_API bool TryGetVec3(const std::string& name, glm::vec3& outValue) const;
        GNS_API bool TryGetVec4(const std::string& name, glm::vec4& outValue) const;
        GNS_API bool TryGetColor3(const std::string& name, glm::vec3& outValue) const;
        GNS_API bool TryGetColor4(const std::string& name, glm::vec4& outValue) const;
        GNS_API bool TryGetMat4(const std::string& name, glm::mat4& outValue) const;
        GNS_API bool TryGetTexture(const std::string& name, Reference<gns::Texture>& outTexture) const;

        GNS_API float GetFloat(const std::string& name, float fallback = 0.0f) const;
        GNS_API int32_t GetInt(const std::string& name, int32_t fallback = 0) const;
        GNS_API uint32_t GetUInt(const std::string& name, uint32_t fallback = 0) const;
        GNS_API glm::vec2 GetVec2(const std::string& name, const glm::vec2& fallback = glm::vec2(0.0f)) const;
        GNS_API glm::vec3 GetVec3(const std::string& name, const glm::vec3& fallback = glm::vec3(0.0f)) const;
        GNS_API glm::vec4 GetVec4(const std::string& name, const glm::vec4& fallback = glm::vec4(0.0f)) const;
        GNS_API glm::vec3 GetColor3(const std::string& name, const glm::vec3& fallback = glm::vec3(0.0f)) const;
        GNS_API glm::vec4 GetColor4(const std::string& name, const glm::vec4& fallback = glm::vec4(0.0f)) const;
        GNS_API glm::mat4 GetMat4(const std::string& name, const glm::mat4& fallback = glm::mat4(1.0f)) const;
        GNS_API Reference<gns::Texture> GetTexture(
            const std::string& name,
            Reference<gns::Texture> fallback = {}) const;
        GNS_API Handle GetTextureHandle(const std::string& name, Handle fallback = {}) const;
        GNS_API size_t GetTextureSlotCount() const;
        GNS_API const MaterialPropertyInfo* GetTextureSlotProperty(size_t slotIndex) const;
        GNS_API bool TryGetTexture(size_t slotIndex, Reference<gns::Texture>& outTexture) const;
        GNS_API Handle GetTextureHandle(size_t slotIndex, Handle fallback = {}) const;

        GNS_API float* GetFloatPtr(const std::string& name);
        GNS_API int32_t* GetIntPtr(const std::string& name);
        GNS_API uint32_t* GetUIntPtr(const std::string& name);
        GNS_API glm::vec2* GetVec2Ptr(const std::string& name);
        GNS_API glm::vec3* GetVec3Ptr(const std::string& name);
        GNS_API glm::vec4* GetVec4Ptr(const std::string& name);
        GNS_API glm::vec3* GetColor3Ptr(const std::string& name);
        GNS_API glm::vec4* GetColor4Ptr(const std::string& name);
        GNS_API glm::mat4* GetMat4Ptr(const std::string& name);

        GNS_API const float* GetFloatPtr(const std::string& name) const;
        GNS_API const int32_t* GetIntPtr(const std::string& name) const;
        GNS_API const uint32_t* GetUIntPtr(const std::string& name) const;
        GNS_API const glm::vec2* GetVec2Ptr(const std::string& name) const;
        GNS_API const glm::vec3* GetVec3Ptr(const std::string& name) const;
        GNS_API const glm::vec4* GetVec4Ptr(const std::string& name) const;
        GNS_API const glm::vec3* GetColor3Ptr(const std::string& name) const;
        GNS_API const glm::vec4* GetColor4Ptr(const std::string& name) const;
        GNS_API const glm::mat4* GetMat4Ptr(const std::string& name) const;

        template<typename T>
        T* GetValuePtr(const std::string& name)
        {
            static_assert(MaterialValueTypeResolver<T>::Supported, "Unsupported material value pointer type.");

            MaterialPropertyInfo property;
            if (!TryGetProperty(name, property) ||
                property.type != MaterialValueTypeResolver<T>::Type ||
                property.elementCount != 1 ||
                property.elementSize != sizeof(T))
            {
                return nullptr;
            }

            return static_cast<T*>(GetMutablePropertyData(property));
        }

        template<typename T>
        const T* GetValuePtr(const std::string& name) const
        {
            static_assert(MaterialValueTypeResolver<T>::Supported, "Unsupported material value pointer type.");

            MaterialPropertyInfo property;
            if (!TryGetProperty(name, property) ||
                property.type != MaterialValueTypeResolver<T>::Type ||
                property.elementCount != 1 ||
                property.elementSize != sizeof(T))
            {
                return nullptr;
            }

            return static_cast<const T*>(GetPropertyData(property));
        }

        GNS_API const std::vector<MaterialPropertyInfo>& GetProperties() const;
        GNS_API MaterialDataBlob GetDataBlob() const;
        GNS_API const void* GetPropertyData(const MaterialPropertyInfo& property) const;
        GNS_API void* GetMutablePropertyData(const MaterialPropertyInfo& property);

        GNS_API void ClearProperties();

    private:
        MaterialLayout m_layout;
        std::vector<uint8_t> m_dataBlob;
        std::vector<Reference<gns::Texture>> m_textureSlots;
        std::vector<size_t> m_texturePropertyIndices;
        std::unordered_map<std::string, size_t> m_textureNameToSlot;

        bool EnsureProperty(
            const std::string& name,
            MaterialPropertyType type,
            uint32_t elementCount);
        void RebuildTextureSlots(
            const std::unordered_map<std::string, Reference<gns::Texture>>* preservedTextures = nullptr);
    };
}
