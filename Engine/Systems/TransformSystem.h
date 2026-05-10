#pragma once

#include "system.h"

#include <vector>

#include <glm/glm.hpp>

#include "../Core/Entity.h"

class TransformSystem : public gns::core::System
{
    void OnCreate() override;
    void OnStart() override;
    void OnEnable() override;
    void OnUpdate(float deltaTime) override;
    void OnFixedUpdate() override;
    void OnDisable() override;
    void OnDestroy() override;
    void OnLateUpdate(float deltaTime) override;

public:
    TransformSystem() = default;
    ~TransformSystem();

private:
    struct TransformTraversalNode
    {
        gns::entityHandle entity = gns::NullEntity;
        glm::mat4 parentMatrix = glm::mat4(1.0f);
    };

    std::vector<TransformTraversalNode> m_traversalStack;
};
