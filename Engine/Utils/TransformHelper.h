#pragma once
#include <glm/vec3.hpp>

#include "../Engine.h"
#include "../Core/ComponentLibrary.h"

namespace gns
{
    
class TransformHelper
{
public:
    
    static constexpr glm::vec3 WorldForward = glm::vec3(0.0f, 0.0f, 1.0f);
    static constexpr glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    static constexpr glm::vec3 WorldRight = glm::vec3(-1.0f, 0.0f, 1.0f);
    
    GNS_API static glm::vec3 Forward(Transform& transform);
    GNS_API static glm::vec3 Up(Transform& transform);
    GNS_API static glm::vec3 Right(Transform& transform);
    
};
}
