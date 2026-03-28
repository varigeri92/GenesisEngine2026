#pragma once
#include "system.h"

class TransformSystem: public gns::core::System
{
    // Inherited via System
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
};
