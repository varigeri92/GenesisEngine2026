#include "gnspch.h"
#include "VulkanShader.h"
#include "../Vulkan/Device.h"

void gns::rendering::VulkanShader::Destroy()
{
    if (m_device != nullptr)
    {
        m_device->DestroyShader(*this);
    }
}
