#pragma once
#include "Genesis.h"

namespace gns
{
    class RenderSystem;
}

class EditorCameraSystem : public gns::core::System
{
    CameraBackend m_cameraBackend = {};
    float pitch = 0.0f;
    float yaw = 0.0f;
public:
    Camera m_camera = {
        .fov = 55.f,
        .near = 0.001f,
        .far = 1000.0f,
        .aspect = 1920.f/1080.f,
    };
    glm::vec3 m_position = glm::vec3(0.0f, 0.0f, -10.0f);
    glm::vec3 m_rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    float m_cameraSpeed = 5.f;
    
    gns::RenderSystem* m_renderSystem = nullptr;
    void OnCreate() override;
    void OnUpdate(float deltaTime) override;
    void SetViewYXZ(glm::vec3 position, glm::vec3 rotation);
    
    void InitCamera();
    void UpdateCamera(float deltaTime);
    
    bool test = false;
};
