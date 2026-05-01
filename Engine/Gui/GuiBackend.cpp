#include "gnspch.h"
#include "GuiBackend.h"
#include <iterator>
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_vulkan.h"
#include "../Renderer/RenderSystem.h"
#include "../Renderer/Renderer.h"


gns::gui::GuiBackend::GuiBackend()
{
}

gns::gui::GuiBackend::~GuiBackend() = default;

void gns::gui::GuiBackend::DrawImGui(VkCommandBuffer cmd, const VkRenderingInfo& renderInfo)
{
	vkCmdBeginRendering(cmd, &renderInfo);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
	
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
	
	vkCmdEndRendering(cmd);
}

void gns::gui::GuiBackend::HandleEvents(SDL_Event& event)
{
	ImGui_ImplSDL2_ProcessEvent(&event);
}

void gns::gui::GuiBackend::OnCreate(SDL_Window* window, gns::RenderSystem& renderSystem)
{
	gns::rendering::Renderer& renderer = renderSystem.GetRenderer();

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = renderer.GetInstance();
	init_info.PhysicalDevice = renderer.GetPhysicalDevice();
	init_info.Device = renderer.GetDevice();
	init_info.Queue = renderer.GetGraphicsQueue();
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = renderer.GetSwapChainFormat();

	m_device = init_info.Device;
	VkDescriptorPoolSize pool_sizes[] = { 
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	if (vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_imguiPool) != VK_SUCCESS)
	{
		LOG_ERROR("[GuiBackend]: Failed to create ImGui descriptor pool.");
		return;
	}
	ImGui::CreateContext();
	if (!ImGui_ImplSDL2_InitForVulkan(window))
	{
		LOG_ERROR("[GuiBackend]: Failed to initialize ImGui SDL Vulkan backend.");
		return;
	}
	
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.DescriptorPool = m_imguiPool;
	if (!ImGui_ImplVulkan_Init(&init_info))
	{
		LOG_ERROR("[GuiBackend]: Failed to initialize ImGui Vulkan backend.");
		return;
	}
	auto& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
}

void gns::gui::GuiBackend::BeginGuiFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void gns::gui::GuiBackend::OnUpdate()
{
	ImGui::ShowDemoWindow();
}

void gns::gui::GuiBackend::OnEndGuiFrame()
{
	ImGui::Render();
}


void gns::gui::GuiBackend::OnDestroy()
{
	ImGui_ImplVulkan_Shutdown();
	if (m_imguiPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_device, m_imguiPool, nullptr);
		m_imguiPool = VK_NULL_HANDLE;
	}
}
