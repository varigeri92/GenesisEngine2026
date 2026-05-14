#include "gnspch.h"
#include "VulkanMaterial.h"

void gns::rendering::VulkanMaterial::Destroy()
{
    materialDataBuffer.reset();
    materialDataSet = VK_NULL_HANDLE;
    textureSet = VK_NULL_HANDLE;
    materialDataLayout = VK_NULL_HANDLE;
    textureLayout = VK_NULL_HANDLE;
    materialDataSetIndex = gns::InvalidMaterialBinding;
    materialDataBinding = gns::InvalidMaterialBinding;
    materialDataDescriptorKind = MaterialDescriptorKind::None;
    textureSetIndex = gns::InvalidMaterialBinding;
    materialDataCache.clear();
    textureBindings.clear();
    vkShader = nullptr;
}
