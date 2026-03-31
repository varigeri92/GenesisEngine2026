#include "gnspch.h"
#include "Mesh.h"

#include "../Renderer/RenderSystem.h"
#include "../Renderer/Vulkan/VulkanMesh.h"
#include "../Systems/SystemsManager.h"

void gns::Mesh::Apply(bool cpuReadWrite /*=false */)
{
    RenderSystem* render_system = core::SystemsManager::GetSystem<RenderSystem>();
    render_system->GetRenderer().ApplyMesh(*this);
    if (!cpuReadWrite)
        FreeCPUSide();
}

void gns::Mesh::FreeCPUSide()
{
    indices.clear();
    indices.reserve(0);
    positions.clear();
    positions.reserve(0);
    colors.clear();
    colors.reserve(0);
    normals.clear();
    normals.reserve(0);
    tangents.clear();
    tangents.reserve(0);
    bitangents.clear();
    bitangents.reserve(0);
}
