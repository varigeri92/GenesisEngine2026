#include "gnspch.h"
#include "RenderGraph.h"

#include <utility>

#include "Vulkan/vkutils.h"
#include "Vulkan/VulkanImage.h"

gns::rendering::RenderStep::RenderStep(
    std::string name,
    RenderStepFunction renderPassFunction)
{
    m_name = std::move(name);
    m_renderPassFunction = std::move(renderPassFunction);
}

void gns::rendering::RenderStep::ExecuteRenderPass(VkCommandBuffer cmd, FrameData& frameData)
{
    if (!m_renderPassFunction(cmd, data, frameData))
    {
        LOG_ERROR("Error While Executing RenderPass - '" + m_name + "'");
    }
}

gns::rendering::RenderStep& gns::rendering::RenderGraph::AddPass(
    std::string name,
    RenderStepFunction renderPassFunction)
{
    m_renderSteps.emplace_back(std::move(name), std::move(renderPassFunction));
    return m_renderSteps[m_renderSteps.size() - 1];
}

gns::rendering::RenderStep& gns::rendering::RenderGraph::AddImageTransitionPass(
    std::string name,
    VulkanImage* image,
    VkImageLayout srcLayout,
    VkImageLayout dstLayout)
{
    RenderStep& step = AddPass(std::move(name),
        [](VkCommandBuffer cmd, RenderStepData& rp_data, FrameData& frameData)
    {
        utils::TransitionImage(
            cmd,
            rp_data.renderTarget->image,
            rp_data.srcImageLayout,
            rp_data.dstImageLayout);
        return true;
    });
    step.data.renderTarget = image;
    step.data.srcImageLayout = srcLayout;
    step.data.dstImageLayout = dstLayout;
    return step;
}

void gns::rendering::RenderGraph::Execute(VkCommandBuffer cmd, FrameData& frameData)
{
    for (RenderStep& renderStep : m_renderSteps)
    {
        renderStep.ExecuteRenderPass(cmd, frameData);
    }
}

void gns::rendering::RenderGraph::Clear()
{
    m_renderSteps.clear();
}
