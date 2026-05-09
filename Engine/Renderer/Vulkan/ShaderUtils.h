#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "../../API/API.h"

namespace gns::rendering
{
    enum class ShaderResourceKind
    {
        Unknown,
        Input,
        Output,
        UniformBuffer,
        StorageBuffer,
        Texture,
        Sampler,
        StorageImage,
        PushConstant
    };

    struct ShaderBlockMemberInfo
    {
        std::string name;
        std::string type;
        uint32_t offset = 0;
        uint32_t size = 0;
        std::vector<ShaderBlockMemberInfo> members;
    };

    struct ShaderResourceInfo
    {
        std::string name;
        std::string type;
        ShaderResourceKind kind = ShaderResourceKind::Unknown;
        VkShaderStageFlags stageFlags = 0;
        VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
        uint32_t set = 0;
        uint32_t binding = 0;
        uint32_t location = 0;
        uint32_t offset = 0;
        uint32_t size = 0;
        uint32_t count = 1;
        std::vector<ShaderBlockMemberInfo> members;
    };

    struct ShaderReflectionData
    {
        std::string filePath;
        std::string entryPoint;
        VkShaderStageFlagBits stage = VK_SHADER_STAGE_ALL;
        std::vector<ShaderResourceInfo> descriptors;
        std::vector<ShaderResourceInfo> pushConstants;
        std::vector<ShaderResourceInfo> inputs;
        std::vector<ShaderResourceInfo> outputs;
    };

    class ShaderUtils
    {
    public:
        GNS_API static std::string ResolveCompiledShaderPath(const std::string& shaderPath);
        GNS_API static bool ReflectShaderFile(const std::string& shaderPath, ShaderReflectionData& outReflection);
        GNS_API static void PrintReflection(const ShaderReflectionData& reflection);

        GNS_API static const char* ToString(ShaderResourceKind kind);
        GNS_API static const char* ToString(VkDescriptorType descriptorType);
        GNS_API static const char* ToString(VkShaderStageFlagBits stage);
    };
}
