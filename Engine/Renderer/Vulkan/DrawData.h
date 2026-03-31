#pragma once
#include <glm/glm.hpp>
#include "../Resources/VulkanShader.h"
#include "VulkanBuffer.h"

namespace gns
{
struct DrawData
{
    glm::mat4 transform;
    VkBuffer vk_indexBuffer;
    gns::rendering::VulkanShader vkShader; 
    VkDeviceAddress vk_vertexBufferAddress;
    size_t StartIndex;
    size_t Count;
};
}
