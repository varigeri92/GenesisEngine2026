#include "gnspch.h"
#include "Shader.h"

#include "RenderSystem.h"
#include "../Systems/SystemsManager.h"

gns::Shader::Shader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath, const std::string& name) : 
Object(name), m_vertexShaderPath(vertexShaderPath), m_fragmentShaderPath(fragmentShaderPath)
{
    LOG_INFO(m_vertexShaderPath);
    LOG_INFO(m_fragmentShaderPath);
}

void gns::Shader::Dispose()
{
    Object::Dispose();
}

gns::Shader::~Shader()
{
}

gns::Handle gns::Shader::CreateVulkanShader()
{
    gns::RenderSystem* render_system = core::SystemsManager::GetSystem<RenderSystem>();
    return render_system->CreateVulkanShader(*this);
    
}
