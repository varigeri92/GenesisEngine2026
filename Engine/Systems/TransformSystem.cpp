#include "gnspch.h"
#include "TransformSystem.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../Core/ComponentLibrary.h"
#include "SystemsManager.h"
#include "entt/entt.hpp"

void TransformSystem::OnCreate()
{
}

void TransformSystem::OnStart()
{
}

void TransformSystem::OnEnable()
{
}

void TransformSystem::OnUpdate(float deltaTime)
{
    auto view = gns::core::SystemsManager::GetRegistry().view<Transform>();
    view.each([](Transform& transform)
    {
        const glm::mat4 translation = glm::translate(glm::mat4(1.0f), transform.position);
        const glm::mat4 rotation = glm::mat4_cast(glm::quat(glm::radians(transform.rotation)));
        const glm::mat4 scale = glm::scale(glm::mat4(1.0f), transform.scale);

        transform.matrix = translation * rotation * scale;
    });
}

void TransformSystem::OnFixedUpdate()
{
}

void TransformSystem::OnDisable()
{
}

void TransformSystem::OnDestroy()
{
}

void TransformSystem::OnLateUpdate(float deltaTime)
{
}

TransformSystem::~TransformSystem() = default;
