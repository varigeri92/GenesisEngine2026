#include "gnspch.h"
#include "TransformSystem.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../Core/ComponentLibrary.h"
#include "../Scene/SceneManager.h"
#include "SystemsManager.h"

namespace
{
    glm::mat4 BuildLocalTransformMatrix(const Transform& transform)
    {
        const glm::mat4 translation = glm::translate(glm::mat4(1.0f), transform.position);
        const glm::mat4 rotation = glm::mat4_cast(glm::quat(glm::radians(transform.rotation)));
        const glm::mat4 scale = glm::scale(glm::mat4(1.0f), transform.scale);

        return translation * rotation * scale;
    }
}

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
    const auto& scenes = gns::SceneManager::GetLoadedScenes();

    m_traversalStack.clear();
    if (m_traversalStack.capacity() == 0)
    {
        m_traversalStack.reserve(256);
    }

    for (const auto& scene : scenes)
    {
        if (scene == nullptr || !scene->root.IsValid())
        {
            continue;
        }

        m_traversalStack.push_back({
            .entity = scene->root.entity_handle,
            .parentMatrix = glm::mat4(1.0f)
        });

        while (!m_traversalStack.empty())
        {
            const TransformTraversalNode node = m_traversalStack.back();
            m_traversalStack.pop_back();

            gns::Entity entity(node.entity);
            if (!entity.IsValid())
            {
                continue;
            }

            glm::mat4 worldMatrix = node.parentMatrix;
            if (Transform* transform = entity.TryGetComponent<Transform>())
            {
                worldMatrix = node.parentMatrix * BuildLocalTransformMatrix(*transform);
                transform->matrix = worldMatrix;
            }

            for (gns::entityHandle child : entity.Children())
            {
                m_traversalStack.push_back({
                    .entity = child,
                    .parentMatrix = worldMatrix
                });
            }
        }
    }
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
