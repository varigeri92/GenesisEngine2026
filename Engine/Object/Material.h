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

    enum class MaterialPropertyType : uint8_t
    {
        Unknown,
        Float,
        Int,
        UInt,
        Vec2,
        Vec3,
        Vec4,
        Mat4,
        FloatArray,
        IntArray,
        UIntArray,
        Bytes
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

        bool IsArray() const { return elementCount > 1; }
    };

    struct MaterialDataBlob
    {
        const void* data = nullptr;
        size_t size = 0;

        bool IsValid() const { return data != nullptr && size > 0; }
    };

    struct MaterialLayout
    {
        bool AddProperty(
            const std::string& name,
            MaterialPropertyType type,
            uint32_t elementCount = 1);
        bool AddPropertyAtOffset(
            const std::string& name,
            MaterialPropertyType type,
            size_t offset,
            size_t size,
            uint32_t elementCount = 1,
            size_t elementStride = 0,
            size_t alignment = 0);

        void Clear();
        void SetSize(size_t size);
        bool IsValid() const { return m_size > 0; }
        bool IsCompatibleWith(const MaterialLayout& other) const;
        size_t GetSize() const { return m_size; }
        const std::vector<MaterialPropertyInfo>& GetProperties() const { return m_properties; }
        const MaterialPropertyInfo* FindProperty(const std::string& name) const;

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
        glm::vec4 albedo_color = glm::vec4(1.0f);
        Reference<gns::Texture> albedo_texture;

        Material();
        explicit Material(std::string name);
        Material(Handle handle, std::string name);

        void SetLayout(const MaterialLayout& layout, bool preserveValues = true);
        const MaterialLayout& GetLayout() const { return m_layout; }

        void SetFloat(const std::string& name, float value);
        void SetInt(const std::string& name, int32_t value);
        void SetUInt(const std::string& name, uint32_t value);
        void SetVec2(const std::string& name, const glm::vec2& value);
        void SetVec3(const std::string& name, const glm::vec3& value);
        void SetVec4(const std::string& name, const glm::vec4& value);
        void SetColor3(const std::string& name, const glm::vec3& value);
        void SetColor4(const std::string& name, const glm::vec4& value);
        void SetMat4(const std::string& name, const glm::mat4& value);
        void SetFloatArray(const std::string& name, std::span<const float> values);
        void SetIntArray(const std::string& name, std::span<const int32_t> values);
        void SetUIntArray(const std::string& name, std::span<const uint32_t> values);
        void SetBytes(
            const std::string& name,
            MaterialPropertyType type,
            const void* data,
            size_t size,
            uint32_t elementCount = 1);

        bool TryGetProperty(const std::string& name, MaterialPropertyInfo& outInfo) const;
        bool TryGetFloat(const std::string& name, float& outValue) const;
        bool TryGetInt(const std::string& name, int32_t& outValue) const;
        bool TryGetUInt(const std::string& name, uint32_t& outValue) const;
        bool TryGetVec2(const std::string& name, glm::vec2& outValue) const;
        bool TryGetVec3(const std::string& name, glm::vec3& outValue) const;
        bool TryGetVec4(const std::string& name, glm::vec4& outValue) const;
        bool TryGetColor3(const std::string& name, glm::vec3& outValue) const;
        bool TryGetColor4(const std::string& name, glm::vec4& outValue) const;
        bool TryGetMat4(const std::string& name, glm::mat4& outValue) const;

        float GetFloat(const std::string& name, float fallback = 0.0f) const;
        int32_t GetInt(const std::string& name, int32_t fallback = 0) const;
        uint32_t GetUInt(const std::string& name, uint32_t fallback = 0) const;
        glm::vec2 GetVec2(const std::string& name, const glm::vec2& fallback = glm::vec2(0.0f)) const;
        glm::vec3 GetVec3(const std::string& name, const glm::vec3& fallback = glm::vec3(0.0f)) const;
        glm::vec4 GetVec4(const std::string& name, const glm::vec4& fallback = glm::vec4(0.0f)) const;
        glm::vec3 GetColor3(const std::string& name, const glm::vec3& fallback = glm::vec3(0.0f)) const;
        glm::vec4 GetColor4(const std::string& name, const glm::vec4& fallback = glm::vec4(0.0f)) const;
        glm::mat4 GetMat4(const std::string& name, const glm::mat4& fallback = glm::mat4(1.0f)) const;

        float* GetFloatPtr(const std::string& name) { return GetValuePtr<float>(name); }
        int32_t* GetIntPtr(const std::string& name) { return GetValuePtr<int32_t>(name); }
        uint32_t* GetUIntPtr(const std::string& name) { return GetValuePtr<uint32_t>(name); }
        glm::vec2* GetVec2Ptr(const std::string& name) { return GetValuePtr<glm::vec2>(name); }
        glm::vec3* GetVec3Ptr(const std::string& name) { return GetValuePtr<glm::vec3>(name); }
        glm::vec4* GetVec4Ptr(const std::string& name) { return GetValuePtr<glm::vec4>(name); }
        glm::vec3* GetColor3Ptr(const std::string& name) { return GetValuePtr<glm::vec3>(name); }
        glm::vec4* GetColor4Ptr(const std::string& name) { return GetValuePtr<glm::vec4>(name); }
        glm::mat4* GetMat4Ptr(const std::string& name) { return GetValuePtr<glm::mat4>(name); }

        const float* GetFloatPtr(const std::string& name) const { return GetValuePtr<float>(name); }
        const int32_t* GetIntPtr(const std::string& name) const { return GetValuePtr<int32_t>(name); }
        const uint32_t* GetUIntPtr(const std::string& name) const { return GetValuePtr<uint32_t>(name); }
        const glm::vec2* GetVec2Ptr(const std::string& name) const { return GetValuePtr<glm::vec2>(name); }
        const glm::vec3* GetVec3Ptr(const std::string& name) const { return GetValuePtr<glm::vec3>(name); }
        const glm::vec4* GetVec4Ptr(const std::string& name) const { return GetValuePtr<glm::vec4>(name); }
        const glm::vec3* GetColor3Ptr(const std::string& name) const { return GetValuePtr<glm::vec3>(name); }
        const glm::vec4* GetColor4Ptr(const std::string& name) const { return GetValuePtr<glm::vec4>(name); }
        const glm::mat4* GetMat4Ptr(const std::string& name) const { return GetValuePtr<glm::mat4>(name); }

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

        const std::vector<MaterialPropertyInfo>& GetProperties() const { return m_layout.GetProperties(); }
        MaterialDataBlob GetDataBlob() const { return { m_dataBlob.data(), m_dataBlob.size() }; }
        const void* GetPropertyData(const MaterialPropertyInfo& property) const;

        void ClearProperties();

    private:
        MaterialLayout m_layout;
        std::vector<uint8_t> m_dataBlob;

        bool EnsureProperty(
            const std::string& name,
            MaterialPropertyType type,
            uint32_t elementCount);
        void* GetMutablePropertyData(const MaterialPropertyInfo& property);
    };
}
