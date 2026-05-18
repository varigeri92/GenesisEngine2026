#include "gnspch.h"
#include "GuiBackend.h"
#include <iterator>
#include "imgui.h"
#include "ImGuizmo.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_vulkan.h"
#include "../Renderer/RenderSystem.h"
#include "../Renderer/Renderer.h"
#include "../Utils/Path.h"
#include <filesystem>

namespace
{
	void ApplyGenesisEditorStyle()
	{
		ImGuiStyle& style = ImGui::GetStyle();

		style.Alpha = 1.0f;
		style.DisabledAlpha = 0.46f;
		style.WindowPadding = ImVec2(10.0f, 8.0f);
		style.WindowRounding = 5.0f;
		style.WindowBorderSize = 1.0f;
		style.WindowMinSize = ImVec2(120.0f, 80.0f);
		style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
		style.ChildRounding = 4.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupRounding = 4.0f;
		style.PopupBorderSize = 1.0f;
		style.FramePadding = ImVec2(8.0f, 5.0f);
		style.FrameRounding = 4.0f;
		style.FrameBorderSize = 0.0f;
		style.ItemSpacing = ImVec2(8.0f, 6.0f);
		style.ItemInnerSpacing = ImVec2(6.0f, 5.0f);
		style.CellPadding = ImVec2(8.0f, 5.0f);
		style.IndentSpacing = 18.0f;
		style.ColumnsMinSpacing = 6.0f;
		style.ScrollbarSize = 12.0f;
		style.ScrollbarRounding = 8.0f;
		style.GrabMinSize = 10.0f;
		style.GrabRounding = 4.0f;
		style.TabRounding = 4.0f;
		style.TabBorderSize = 0.0f;
		style.TabCloseButtonMinWidthSelected = 0.0f;
		style.TabCloseButtonMinWidthUnselected = 0.0f;
		style.DockingSeparatorSize = 1.0f;

		ImVec4* colors = style.Colors;
		colors[ImGuiCol_Text] = ImVec4(0.88f, 0.91f, 0.92f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.43f, 0.48f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.075f, 0.085f, 0.96f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.045f, 0.055f, 0.065f, 0.86f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.075f, 0.085f, 0.095f, 0.98f);
		colors[ImGuiCol_Border] = ImVec4(0.20f, 0.25f, 0.26f, 0.72f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.125f, 0.13f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.13f, 0.20f, 0.19f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.12f, 0.33f, 0.30f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.035f, 0.045f, 0.055f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.055f, 0.075f, 0.085f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.035f, 0.045f, 0.055f, 0.92f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.070f, 0.085f, 0.095f, 0.96f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.035f, 0.045f, 0.050f, 0.84f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.26f, 0.27f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.26f, 0.36f, 0.36f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.18f, 0.55f, 0.49f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 0.86f, 0.73f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.22f, 0.72f, 0.64f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.94f, 0.63f, 0.28f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.11f, 0.15f, 0.16f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.36f, 0.34f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.24f, 0.66f, 0.58f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.12f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.38f, 0.35f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.23f, 0.58f, 0.52f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.24f, 0.25f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.86f, 0.51f, 0.25f, 1.00f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.95f, 0.65f, 0.32f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.19f, 0.48f, 0.44f, 0.48f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.27f, 0.75f, 0.67f, 0.78f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.96f, 0.62f, 0.31f, 0.95f);
		colors[ImGuiCol_Tab] = ImVec4(0.075f, 0.095f, 0.105f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.48f, 0.43f, 1.00f);
		colors[ImGuiCol_TabSelected] = ImVec4(0.13f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.30f, 0.90f, 0.75f, 1.00f);
		colors[ImGuiCol_TabDimmed] = ImVec4(0.055f, 0.065f, 0.075f, 1.00f);
		colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.095f, 0.12f, 0.13f, 1.00f);
		colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.22f, 0.46f, 0.42f, 1.00f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.22f, 0.80f, 0.68f, 0.70f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.45f, 0.76f, 0.76f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.95f, 0.58f, 0.30f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.84f, 0.50f, 0.26f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.98f, 0.69f, 0.36f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.08f, 0.11f, 0.12f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.20f, 0.26f, 0.27f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.12f, 0.16f, 0.17f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09f, 0.11f, 0.12f, 0.34f);
		colors[ImGuiCol_TextLink] = ImVec4(0.28f, 0.82f, 0.72f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.21f, 0.58f, 0.52f, 0.38f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.98f, 0.72f, 0.32f, 0.90f);
		colors[ImGuiCol_NavCursor] = ImVec4(0.30f, 0.90f, 0.76f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.85f, 0.91f, 0.92f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.02f, 0.025f, 0.030f, 0.68f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.02f, 0.025f, 0.030f, 0.76f);
	}

	void LoadGenesisEditorFonts()
	{
		ImGuiIO& io = ImGui::GetIO();

		std::filesystem::path textFontPath =
			gns::path::Resolve(gns::path::Root::EditorResources, "Fonts/geist-static/Geist-Regular.ttf");
		const std::string textFontPathString = textFontPath.string();
		if (io.Fonts->AddFontFromFileTTF(textFontPathString.c_str(), 16.0f) == nullptr)
		{
			LOG_WARNING("[GuiBackend]: Failed to load Geist editor font. Falling back to ImGui default font.");
			io.Fonts->AddFontDefault();
		}

		ImFontConfig iconConfig = {};
		iconConfig.MergeMode = true;
		iconConfig.PixelSnapH = true;
		iconConfig.GlyphMinAdvanceX = 18.0f;
		iconConfig.GlyphOffset = ImVec2(0.0f, 2.0f);

		static const ImWchar iconRanges[] =
		{
			0xe000, 0xf8ff,
			0
		};

		std::filesystem::path iconFontPath =
			gns::path::Resolve(gns::path::Root::EditorResources, "Fonts/MaterialIconsRound-Regular.otf");
		const std::string iconFontPathString = iconFontPath.string();
		if (io.Fonts->AddFontFromFileTTF(iconFontPathString.c_str(), 18.0f, &iconConfig, iconRanges) == nullptr)
		{
			LOG_WARNING("[GuiBackend]: Failed to load Material Icons Round font.");
		}
	}
}


gns::gui::GuiBackend::GuiBackend()
{
}

gns::gui::GuiBackend::~GuiBackend() = default;

void gns::gui::GuiBackend::DrawImGui(VkCommandBuffer cmd, const VkRenderingInfo& renderInfo)
{
	vkCmdBeginRendering(cmd, &renderInfo);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
	vkCmdEndRendering(cmd);
	
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::RenderPlatformWindowsDefault();
	}
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
	ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
	LoadGenesisEditorFonts();
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
	ApplyGenesisEditorStyle();
}

void gns::gui::GuiBackend::BeginGuiFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
}

void gns::gui::GuiBackend::OnUpdate()
{
	ImGui::ShowDemoWindow();
}

void gns::gui::GuiBackend::OnEndGuiFrame()
{
	ImGui::Render();

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
	}
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
