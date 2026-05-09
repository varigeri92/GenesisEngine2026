#pragma once
#include <cstdint>
#include <span>
#include <string>
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
        uint32_t elementCount = 1;
    };

    struct MaterialDataBlob
    {
        const void* data = nullptr;
        size_t size = 0;

        bool IsValid() const { return data != nullptr && size > 0; }
    };

    struct Material : public Object
    {
        Reference<gns::Shader> shader_ref;
        glm::vec4 albedo_color = glm::vec4(1.0f);
        Reference<gns::Texture> albedo_texture;

        Material();
        explicit Material(std::string name);
        Material(Handle handle, std::string name);

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
        const std::vector<MaterialPropertyInfo>& GetProperties() const { return m_properties; }
        MaterialDataBlob GetDataBlob() const { return { m_dataBlob.data(), m_dataBlob.size() }; }
        const void* GetPropertyData(const MaterialPropertyInfo& property) const;

        void ClearProperties();

    private:
        std::vector<MaterialPropertyInfo> m_properties;
        std::vector<uint8_t> m_dataBlob;
    };
}
