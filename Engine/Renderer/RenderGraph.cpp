#include "gnspch.h"
#include "RenderGraph.h"

#include <utility>

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
