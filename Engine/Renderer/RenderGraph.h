#pragma once
#include <functional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace gns::rendering
{
    struct FrameData;
    class VulkanImage;
    struct VulkanShader;

    struct RenderStepData
    {
        VulkanShader* shaderOverride = nullptr;
        VulkanImage* renderTarget = nullptr;
        VulkanImage* depthTarget = nullptr;
        bool randomBool = true;
        VkImageLayout srcImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout dstImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkRenderingInfo renderingInfo = {};
        VkRenderingAttachmentInfo colorAttachment = {};
        VkRenderingAttachmentInfo depthAttachment = {};
    };

    using RenderStepFunction = std::function<bool(VkCommandBuffer, RenderStepData&, FrameData&)>;

    struct RenderStep
    {
        RenderStepData data;
        std::string m_name;

        RenderStep() = default;
        RenderStep(std::string name, RenderStepFunction renderPassFunction);
        void ExecuteRenderPass(VkCommandBuffer cmd, FrameData& frameData);
    private:
        RenderStepFunction m_renderPassFunction;
    };

    class RenderGraph
    {
    public:
        RenderStep& AddPass(std::string name, RenderStepFunction renderPassFunction);
        void Execute(VkCommandBuffer cmd, FrameData& frameData);
        void Clear();
    private:
        std::vector<RenderStep> m_renderSteps;
    };
}
