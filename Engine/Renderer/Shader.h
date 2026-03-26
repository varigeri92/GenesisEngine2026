#pragma once
#include "../Core/Handles.h"
#include "../Object/IObject.h"

namespace gns
{
struct Shader : public Object
{
private:
    std::string m_vertexShaderPath;
    std::string m_fragmentShaderPath;
    Handle m_vulkanShaderHandle;    
public:
    Shader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath, const std::string& name);
    void Dispose() override;
    ~Shader() override;
    std::string GetVertexShaderPath() const { return m_vertexShaderPath; }
    std::string GetFragmentShaderPath() const { return m_fragmentShaderPath; }
    Handle GetVulkanShaderHandle() const { return m_vulkanShaderHandle; }
};
}
