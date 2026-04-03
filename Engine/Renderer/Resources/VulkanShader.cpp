#include "gnspch.h"
#include "VulkanShader.h"
#include "../Vulkan/Device.h"
gns::rendering::VulkanShader::~VulkanShader()
{
    Destroy();
}

void gns::rendering::VulkanShader::Destroy()
{
    m_device->DestroyShader(*this);
}
