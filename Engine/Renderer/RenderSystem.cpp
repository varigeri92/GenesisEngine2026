#include "gnspch.h"
#include "RenderSystem.h"

#include "../Assets/AssetManager.h"
#include "../Window/WindowSystem.h"
#include "../Core/Entity.h"
#include "../Object/Mesh.h"
#include "../Utils/Path.h"
#include "../Core/ComponentLibrary.h"

gns::RenderSystem::RenderSystem(gns::window::WindowSystem* ws) : m_windowSystem(ws), m_renderer(){}

void gns::RenderSystem::OnCreate()
{
	m_renderer.CreateDevice(m_windowSystem->GetSDLWindow());
	LOG_INFO("Render System created!");
}

void gns::RenderSystem::OnStart()
{
	/* 
	gns::Entity entity_1 = gns::Entity::CreateEntity("test_entity_1");
	gns::Entity::CreateEntity("test_entity_2");
	gns::Entity::CreateEntity("test_entity_3");
	gns::Entity entity_4 = gns::Entity::CreateEntity("test_entity_4");

	std::string file = R"(D:\__ProjectGenesis\GenesisEngine_TestProject\Assets\basicmesh.glb)";
	test_assimp(file);
	*/
	
	std::string fragmentShaderPath = gns::path::InResourcesDirectory(R"(Shaders\default.frag)").string();
	std::string vertexShaderPath = gns::path::InResourcesDirectory(R"(Shaders\mesh.vert)").string();
	Shader* _shader = Object::Create<Shader>(vertexShaderPath, fragmentShaderPath, "default_mesh_shader");
	_shader->CreateVulkanShader();
	
	std::vector<gns::assets::LoadedObject> loaded 
		= assets::AssetManager::LoadAsset(R"(D:\__ProjectGenesis\GenesisEngine_TestProject\Assets\basicmesh.glb)");
	for (auto& loaded_object : loaded)
	{
		Mesh* mesh = loaded_object.As<Mesh>();
		mesh->Apply();
		std::string name = mesh->GetName();
		gns::Entity entity = gns::Entity::CreateEntity(name);
		MeshComponent& mesh_comp = entity.AddComponent<MeshComponent>();
		mesh_comp.mesh = mesh->Ref<Mesh>();
		mesh_comp.shader = _shader->Ref<Shader>();
		LOG_INFO(name);
	}
}

void gns::RenderSystem::OnEnable()
{
}

void gns::RenderSystem::OnUpdate(float deltaTime)
{
}

void gns::RenderSystem::OnLateUpdate(float deltaTime)
{
	m_renderer.DrawFrame();
}

void gns::RenderSystem::OnFixedUpdate()
{
}

void gns::RenderSystem::OnDisable()
{
}

void gns::RenderSystem::OnDestroy()
{
}

gns::rendering::Renderer& gns::RenderSystem::GetRenderer()
{
	return m_renderer;
}

void gns::RenderSystem::WaitForIdle()
{
	m_renderer.WaitForIdle();
}
