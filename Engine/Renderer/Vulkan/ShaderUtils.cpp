#include "gnspch.h"
#include "ShaderUtils.h"

#include <fstream>
#include <map>
#include <sstream>

#include <spirv_reflect.h>

#include "../../Utils/Path.h"
#include "DescriptorLayoutBuilder.h"

namespace gns::rendering
{
    namespace
    {
        bool CheckReflectResult(SpvReflectResult result, const std::string& context)
        {
            if (result == SPV_REFLECT_RESULT_SUCCESS)
            {
                return true;
            }

            LOG_ERROR("[ShaderUtils]: SPIR-V reflection failed: " + context);
            return false;
        }

        std::string SafeName(const char* name, const std::string& fallback)
        {
            return name != nullptr && name[0] != '\0' ? name : fallback;
        }

        std::string TypeName(const SpvReflectBlockVariable& variable)
        {
            if (variable.type_description == nullptr)
            {
                return "unknown";
            }

            const char* reflectedName = spvReflectBlockVariableTypeName(&variable);
            return SafeName(reflectedName, "unknown");
        }

        std::string TypeName(const SpvReflectDescriptorBinding& binding)
        {
            if (binding.type_description != nullptr && binding.type_description->type_name != nullptr)
            {
                return binding.type_description->type_name;
            }

            if (binding.type_description != nullptr && binding.type_description->struct_member_name != nullptr)
            {
                return binding.type_description->struct_member_name;
            }

            return TypeName(binding.block);
        }

        std::string TypeName(const SpvReflectInterfaceVariable& variable)
        {
            if (variable.type_description != nullptr && variable.type_description->type_name != nullptr)
            {
                return variable.type_description->type_name;
            }

            if (variable.type_description != nullptr && variable.type_description->struct_member_name != nullptr)
            {
                return variable.type_description->struct_member_name;
            }

            return "unknown";
        }

        ShaderResourceKind KindFromDescriptorType(SpvReflectDescriptorType descriptorType)
        {
            switch (descriptorType)
            {
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                return ShaderResourceKind::UniformBuffer;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                return ShaderResourceKind::StorageBuffer;
            case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                return ShaderResourceKind::Texture;
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
                return ShaderResourceKind::Sampler;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                return ShaderResourceKind::StorageImage;
            default:
                return ShaderResourceKind::Unknown;
            }
        }

        uint32_t DescriptorArrayCount(const SpvReflectDescriptorBinding& binding)
        {
            if (binding.array.dims_count == 0)
            {
                return binding.count == 0 ? 1 : binding.count;
            }

            uint32_t count = 1;
            for (uint32_t i = 0; i < binding.array.dims_count; ++i)
            {
                count *= binding.array.dims[i];
            }
            return count;
        }

        void AddBlockMembers(
            const SpvReflectBlockVariable& block,
            std::vector<ShaderBlockMemberInfo>& outMembers)
        {
            outMembers.reserve(outMembers.size() + block.member_count);
            for (uint32_t i = 0; i < block.member_count; ++i)
            {
                const SpvReflectBlockVariable& member = block.members[i];

                ShaderBlockMemberInfo memberInfo;
                memberInfo.name = SafeName(member.name, "unnamed");
                memberInfo.type = TypeName(member);
                memberInfo.offset = member.offset;
                memberInfo.size = member.size;

                AddBlockMembers(member, memberInfo.members);
                outMembers.emplace_back(std::move(memberInfo));
            }
        }

        void PrintBlockMembers(
            const std::vector<ShaderBlockMemberInfo>& members,
            const std::string& prefix,
            VkShaderStageFlagBits stage)
        {
            for (const ShaderBlockMemberInfo& member : members)
            {
                const std::string fieldName = prefix.empty()
                    ? member.name
                    : prefix + "." + member.name;

                std::stringstream message;
                message
                    << "[ShaderUtils]: field_name=\"" << fieldName << "\""
                    << " stage=" << ShaderUtils::ToString(stage)
                    << " type=\"" << member.type << "\""
                    << " offset=" << member.offset
                    << " size=" << member.size;
                LOG_INFO(message.str());

                PrintBlockMembers(member.members, fieldName, stage);
            }
        }

        bool ReadSpirvFile(const std::string& shaderPath, std::vector<uint32_t>& outCode)
        {
            std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);
            if (!file.is_open())
            {
                LOG_ERROR("[ShaderUtils]: Failed to open shader file: " + shaderPath);
                return false;
            }

            const size_t fileSize = static_cast<size_t>(file.tellg());
            if (fileSize == 0 || fileSize % sizeof(uint32_t) != 0)
            {
                LOG_ERROR("[ShaderUtils]: Invalid SPIR-V file size: " + shaderPath);
                return false;
            }

            outCode.resize(fileSize / sizeof(uint32_t));
            file.seekg(0);
            file.read(reinterpret_cast<char*>(outCode.data()), fileSize);
            return true;
        }

        struct ReflectedDescriptorBinding
        {
            VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
            uint32_t descriptorCount = 1;
            VkShaderStageFlags stageFlags = 0;
        };
    }

    std::string ShaderUtils::ResolveCompiledShaderPath(const std::string& shaderPath)
    {
        std::string resolvedShaderPath = shaderPath;
        if (!gns::path::HasExtension(resolvedShaderPath, "spv"))
        {
            resolvedShaderPath += ".spv";
        }

        return gns::path::Resolve(gns::path::Root::EditorResources, resolvedShaderPath).string();
    }

    bool ShaderUtils::ReflectShaderFile(const std::string& shaderPath, ShaderReflectionData& outReflection)
    {
        std::vector<uint32_t> spirvCode;
        if (!ReadSpirvFile(shaderPath, spirvCode))
        {
            return false;
        }

        SpvReflectShaderModule module{};
        const SpvReflectResult result = spvReflectCreateShaderModule(
            spirvCode.size() * sizeof(uint32_t),
            spirvCode.data(),
            &module);
        if (!CheckReflectResult(result, shaderPath))
        {
            return false;
        }

        outReflection = {};
        outReflection.filePath = shaderPath;
        outReflection.entryPoint = SafeName(module.entry_point_name, "main");
        outReflection.stage = static_cast<VkShaderStageFlagBits>(module.shader_stage);

        uint32_t descriptorCount = 0;
        if (CheckReflectResult(spvReflectEnumerateDescriptorBindings(&module, &descriptorCount, nullptr), "descriptor count"))
        {
            std::vector<SpvReflectDescriptorBinding*> bindings(descriptorCount);
            if (descriptorCount == 0 ||
                CheckReflectResult(spvReflectEnumerateDescriptorBindings(&module, &descriptorCount, bindings.data()), "descriptors"))
            {
                outReflection.descriptors.reserve(descriptorCount);
                for (const SpvReflectDescriptorBinding* binding : bindings)
                {
                    if (binding == nullptr)
                    {
                        continue;
                    }

                    ShaderResourceInfo resource;
                    resource.name = SafeName(binding->name, binding->block.name != nullptr ? binding->block.name : "unnamed");
                    resource.type = TypeName(*binding);
                    resource.kind = KindFromDescriptorType(binding->descriptor_type);
                    resource.stageFlags = outReflection.stage;
                    resource.descriptorType = static_cast<VkDescriptorType>(binding->descriptor_type);
                    resource.set = binding->set;
                    resource.binding = binding->binding;
                    resource.size = binding->block.size;
                    resource.count = DescriptorArrayCount(*binding);
                    AddBlockMembers(binding->block, resource.members);
                    outReflection.descriptors.emplace_back(std::move(resource));
                }
            }
        }

        uint32_t pushConstantCount = 0;
        if (CheckReflectResult(spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, nullptr), "push constant count"))
        {
            std::vector<SpvReflectBlockVariable*> blocks(pushConstantCount);
            if (pushConstantCount == 0 ||
                CheckReflectResult(spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, blocks.data()), "push constants"))
            {
                outReflection.pushConstants.reserve(pushConstantCount);
                for (const SpvReflectBlockVariable* block : blocks)
                {
                    if (block == nullptr)
                    {
                        continue;
                    }

                    ShaderResourceInfo resource;
                    resource.name = SafeName(block->name, "push_constants");
                    resource.type = TypeName(*block);
                    resource.kind = ShaderResourceKind::PushConstant;
                    resource.stageFlags = outReflection.stage;
                    resource.offset = block->offset;
                    resource.size = block->size;
                    AddBlockMembers(*block, resource.members);
                    outReflection.pushConstants.emplace_back(std::move(resource));
                }
            }
        }

        uint32_t inputCount = 0;
        if (CheckReflectResult(spvReflectEnumerateInputVariables(&module, &inputCount, nullptr), "input count"))
        {
            std::vector<SpvReflectInterfaceVariable*> variables(inputCount);
            if (inputCount == 0 ||
                CheckReflectResult(spvReflectEnumerateInputVariables(&module, &inputCount, variables.data()), "inputs"))
            {
                outReflection.inputs.reserve(inputCount);
                for (const SpvReflectInterfaceVariable* variable : variables)
                {
                    if (variable == nullptr ||
                        (variable->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) != 0)
                    {
                        continue;
                    }

                    ShaderResourceInfo resource;
                    resource.name = SafeName(variable->name, "unnamed");
                    resource.type = TypeName(*variable);
                    resource.kind = ShaderResourceKind::Input;
                    resource.stageFlags = outReflection.stage;
                    resource.location = variable->location;
                    resource.size = variable->numeric.scalar.width / 8;
                    outReflection.inputs.emplace_back(std::move(resource));
                }
            }
        }

        uint32_t outputCount = 0;
        if (CheckReflectResult(spvReflectEnumerateOutputVariables(&module, &outputCount, nullptr), "output count"))
        {
            std::vector<SpvReflectInterfaceVariable*> variables(outputCount);
            if (outputCount == 0 ||
                CheckReflectResult(spvReflectEnumerateOutputVariables(&module, &outputCount, variables.data()), "outputs"))
            {
                outReflection.outputs.reserve(outputCount);
                for (const SpvReflectInterfaceVariable* variable : variables)
                {
                    if (variable == nullptr ||
                        (variable->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) != 0)
                    {
                        continue;
                    }

                    ShaderResourceInfo resource;
                    resource.name = SafeName(variable->name, "unnamed");
                    resource.type = TypeName(*variable);
                    resource.kind = ShaderResourceKind::Output;
                    resource.stageFlags = outReflection.stage;
                    resource.location = variable->location;
                    resource.size = variable->numeric.scalar.width / 8;
                    outReflection.outputs.emplace_back(std::move(resource));
                }
            }
        }

        spvReflectDestroyShaderModule(&module);
        return true;
    }

    bool ShaderUtils::CreateDescriptorSetLayouts(
        VkDevice device,
        const std::vector<ShaderReflectionData>& reflections,
        std::vector<VkDescriptorSetLayout>& outLayouts)
    {
        outLayouts.clear();

        std::map<uint32_t, std::map<uint32_t, ReflectedDescriptorBinding>> reflectedSets;
        uint32_t maxSet = 0;
        bool hasDescriptor = false;

        for (const ShaderReflectionData& reflection : reflections)
        {
            for (const ShaderResourceInfo& descriptor : reflection.descriptors)
            {
                hasDescriptor = true;
                maxSet = std::max(maxSet, descriptor.set);

                ReflectedDescriptorBinding& binding =
                    reflectedSets[descriptor.set][descriptor.binding];
                if (binding.descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM)
                {
                    binding.descriptorType = descriptor.descriptorType;
                    binding.descriptorCount = descriptor.count;
                }
                else if (binding.descriptorType != descriptor.descriptorType ||
                    binding.descriptorCount != descriptor.count)
                {
                    std::stringstream message;
                    message
                        << "[ShaderUtils]: Conflicting reflected descriptor binding at set "
                        << descriptor.set
                        << ", binding "
                        << descriptor.binding;
                    LOG_ERROR(message.str());
                    return false;
                }

                binding.stageFlags |= descriptor.stageFlags;
            }
        }

        if (!hasDescriptor)
        {
            return true;
        }

        outLayouts.reserve(maxSet + 1);
        for (uint32_t setIndex = 0; setIndex <= maxSet; ++setIndex)
        {
            DescriptorLayoutBuilder builder;

            if (const auto set = reflectedSets.find(setIndex); set != reflectedSets.end())
            {
                for (const auto& [bindingIndex, binding] : set->second)
                {
                    builder.AddBinding(
                        bindingIndex,
                        binding.descriptorType,
                        binding.descriptorCount,
                        binding.stageFlags);
                }
            }

            outLayouts.emplace_back(builder.Build(device, 0));
        }

        return true;
    }

    std::vector<VkPushConstantRange> ShaderUtils::BuildPushConstantRanges(
        const std::vector<ShaderReflectionData>& reflections)
    {
        std::vector<VkPushConstantRange> ranges;

        for (const ShaderReflectionData& reflection : reflections)
        {
            for (const ShaderResourceInfo& pushConstant : reflection.pushConstants)
            {
                auto existingRange = std::find_if(
                    ranges.begin(),
                    ranges.end(),
                    [&](const VkPushConstantRange& range)
                    {
                        return range.offset == pushConstant.offset &&
                            range.size == pushConstant.size;
                    });

                if (existingRange != ranges.end())
                {
                    existingRange->stageFlags |= pushConstant.stageFlags;
                    continue;
                }

                VkPushConstantRange range{};
                range.offset = pushConstant.offset;
                range.size = pushConstant.size;
                range.stageFlags = pushConstant.stageFlags;
                ranges.emplace_back(range);
            }
        }

        return ranges;
    }

    void ShaderUtils::PrintReflection(const ShaderReflectionData& reflection)
    {
        LOG_INFO("[ShaderUtils]: Reflecting shader: " + reflection.filePath);
        LOG_INFO(std::string("[ShaderUtils]: stage=") + ToString(reflection.stage) + " entry_point=\"" + reflection.entryPoint + "\"");

        for (const ShaderResourceInfo& descriptor : reflection.descriptors)
        {
            std::stringstream message;
            message
                << "[ShaderUtils]: field_name=\"" << descriptor.name << "\""
                << " layout_set=" << descriptor.set
                << " layout_binding=" << descriptor.binding
                << " stage=" << ToString(reflection.stage)
                << " kind=\"" << ToString(descriptor.kind) << "\""
                << " descriptor_type=\"" << ToString(descriptor.descriptorType) << "\""
                << " type=\"" << descriptor.type << "\""
                << " size=" << descriptor.size
                << " count=" << descriptor.count;
            LOG_INFO(message.str());

            PrintBlockMembers(descriptor.members, descriptor.name, reflection.stage);
        }

        for (const ShaderResourceInfo& pushConstant : reflection.pushConstants)
        {
            std::stringstream message;
            message
                << "[ShaderUtils]: field_name=\"" << pushConstant.name << "\""
                << " stage=" << ToString(reflection.stage)
                << " kind=\"" << ToString(pushConstant.kind) << "\""
                << " offset=" << pushConstant.offset
                << " size=" << pushConstant.size;
            LOG_INFO(message.str());

            PrintBlockMembers(pushConstant.members, pushConstant.name, reflection.stage);
        }

        for (const ShaderResourceInfo& input : reflection.inputs)
        {
            std::stringstream message;
            message
                << "[ShaderUtils]: field_name=\"" << input.name << "\""
                << " location=" << input.location
                << " stage=" << ToString(reflection.stage)
                << " kind=\"" << ToString(input.kind) << "\""
                << " type=\"" << input.type << "\"";
            LOG_INFO(message.str());
        }

        for (const ShaderResourceInfo& output : reflection.outputs)
        {
            std::stringstream message;
            message
                << "[ShaderUtils]: field_name=\"" << output.name << "\""
                << " location=" << output.location
                << " stage=" << ToString(reflection.stage)
                << " kind=\"" << ToString(output.kind) << "\""
                << " type=\"" << output.type << "\"";
            LOG_INFO(message.str());
        }
    }

    const char* ShaderUtils::ToString(ShaderResourceKind kind)
    {
        switch (kind)
        {
        case ShaderResourceKind::Input: return "input";
        case ShaderResourceKind::Output: return "output";
        case ShaderResourceKind::UniformBuffer: return "uniform_buffer";
        case ShaderResourceKind::StorageBuffer: return "storage_buffer";
        case ShaderResourceKind::Texture: return "texture";
        case ShaderResourceKind::Sampler: return "sampler";
        case ShaderResourceKind::StorageImage: return "storage_image";
        case ShaderResourceKind::PushConstant: return "push_constant";
        default: return "unknown";
        }
    }

    const char* ShaderUtils::ToString(VkDescriptorType descriptorType)
    {
        switch (descriptorType)
        {
        case VK_DESCRIPTOR_TYPE_SAMPLER: return "sampler";
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return "combined_image_sampler";
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return "sampled_image";
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return "storage_image";
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return "uniform_texel_buffer";
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return "storage_texel_buffer";
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return "uniform_buffer";
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return "storage_buffer";
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return "uniform_buffer_dynamic";
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return "storage_buffer_dynamic";
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return "input_attachment";
        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return "acceleration_structure";
        default: return "unknown";
        }
    }

    const char* ShaderUtils::ToString(VkShaderStageFlagBits stage)
    {
        switch (stage)
        {
        case VK_SHADER_STAGE_VERTEX_BIT: return "vertex";
        case VK_SHADER_STAGE_FRAGMENT_BIT: return "fragment";
        case VK_SHADER_STAGE_COMPUTE_BIT: return "compute";
        case VK_SHADER_STAGE_GEOMETRY_BIT: return "geometry";
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return "tess_control";
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return "tess_eval";
        default: return "unknown";
        }
    }
}
