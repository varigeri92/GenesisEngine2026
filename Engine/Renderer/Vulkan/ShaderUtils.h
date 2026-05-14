#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "../../API/API.h"
#include "../../Object/Material.h"

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
        uint32_t elementCount = 1;
        uint32_t elementStride = 0;
        uint32_t typeFlags = 0;
        uint32_t scalarWidth = 0;
        uint32_t scalarSignedness = 0;
        uint32_t vectorComponentCount = 0;
        uint32_t matrixColumnCount = 0;
        uint32_t matrixRowCount = 0;
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
        GNS_API static bool CreateDescriptorSetLayouts(
            VkDevice device,
            const std::vector<ShaderReflectionData>& reflections,
            std::vector<VkDescriptorSetLayout>& outLayouts);
        GNS_API static std::vector<VkPushConstantRange> BuildPushConstantRanges(
            const std::vector<ShaderReflectionData>& reflections);
        GNS_API static bool ValidateGlobalDescriptorRules(
            const std::vector<ShaderReflectionData>& reflections,
            uint32_t* outBindingMask = nullptr,
            uint32_t globalSet = 0);
        GNS_API static bool ValidateMaterialDescriptorRules(
            const std::vector<ShaderReflectionData>& reflections,
            uint32_t materialSet = 1,
            uint32_t materialBinding = 0,
            uint32_t materialTextureSet = 2);
        GNS_API static gns::MaterialLayout BuildMaterialLayout(
            const std::vector<ShaderReflectionData>& reflections,
            uint32_t materialSet = 1,
            uint32_t materialBinding = 0,
            uint32_t materialTextureSet = 2);
        GNS_API static void PrintReflection(const ShaderReflectionData& reflection);

        GNS_API static const char* ToString(ShaderResourceKind kind);
        GNS_API static const char* ToString(VkDescriptorType descriptorType);
        GNS_API static const char* ToString(VkShaderStageFlagBits stage);
    };
}
