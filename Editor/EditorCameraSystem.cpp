#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "EditorCameraSystem.h"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "GenesisRendering.h"

void EditorCameraSystem::OnCreate()
{
    InitCamera();
}

void EditorCameraSystem::OnUpdate(float deltaTime)
{
    UpdateCamera(deltaTime);
    m_renderSystem->SetCamera(m_cameraBackend);
}

void EditorCameraSystem::InitCamera()
{
    m_position = { 0,0,-8 };
    m_cameraSpeed = 5;
    m_renderSystem = gns::core::SystemsManager::GetSystem<gns::RenderSystem>();
    
    m_rotation = { pitch, yaw, 0.0f };
    glm::vec3 forward = {
        cosf(pitch) * sinf(yaw),
        sinf(pitch),
        cosf(pitch) * cosf(yaw)
    };
    forward = glm::normalize(forward);

    glm::vec3 worldUp = { 0.0f, 1.0f, 0.0f };
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));
    
    m_cameraBackend.projection = glm::perspective(
        glm::radians(m_camera.fov),
        m_camera.aspect,
        m_camera.far,
        m_camera.near);
    
    m_cameraBackend.view = glm::lookAt(
    m_position,
    m_position + forward,
    up
    );
    
    m_renderSystem->SetCamera(m_cameraBackend);
}

void EditorCameraSystem::UpdateCamera(float deltaTime)
{

    if (gns::core::InputBackend::GetMouseButton(3))
    {
        const float speed = m_cameraSpeed * deltaTime;
        const float mouseSensitivity = 5.f * deltaTime;

        // --- ROTATION ---
        yaw += -gns::core::InputBackend::GetMouseVelocity().x * mouseSensitivity;
        pitch += -gns::core::InputBackend::GetMouseVelocity().y * mouseSensitivity;

        // Clamp pitch (avoid flipping upside-down)
        pitch = glm::clamp(pitch, -1.5f, 1.5f);

        m_rotation = { pitch, yaw, 0.0f };

        // --- DIRECTION VECTORS ---
        glm::vec3 forward = {
            cosf(pitch) * sinf(yaw),
            sinf(pitch),
            cosf(pitch) * cosf(yaw)
        };
        forward = glm::normalize(forward);

        glm::vec3 worldUp = { 0.0f, 1.0f, 0.0f };
        glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
        glm::vec3 up = glm::normalize(glm::cross(right, forward));

        // --- MOVEMENT ---
        if (gns::core::InputBackend::GetKey(SDLK_w)) m_position += forward * speed;
        if (gns::core::InputBackend::GetKey(SDLK_s)) m_position -= forward * speed;
        if (gns::core::InputBackend::GetKey(SDLK_a)) m_position -= right * speed;
        if (gns::core::InputBackend::GetKey(SDLK_d)) m_position += right * speed;
        if (gns::core::InputBackend::GetKey(SDLK_e)) m_position += up * speed;
        if (gns::core::InputBackend::GetKey(SDLK_q)) m_position -= up * speed;

        // --- VIEW MATRIX ---
        m_cameraBackend.view = glm::lookAt(
            m_position,
            m_position + forward,
            up
        );
    }
    m_cameraBackend.projection = glm::perspective(
        glm::radians(m_camera.fov),
        m_camera.aspect,
        m_camera.far,
        m_camera.near);
    
    m_cameraBackend.projection[1][1] *= -1;
    m_cameraBackend.viewProjection = m_cameraBackend.projection * m_cameraBackend.view;
}

void EditorCameraSystem::SetViewYXZ(glm::vec3 position, glm::vec3 rotation)
{
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
    const glm::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
    const glm::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };
    glm::mat4 view = glm::mat4{ 1.f };
    view[0][0] = u.x;
    view[1][0] = u.y;
    view[2][0] = u.z;
    view[0][1] = v.x;
    view[1][1] = v.y;
    view[2][1] = v.z;
    view[0][2] = w.x;
    view[1][2] = w.y;
    view[2][2] = w.z;
    view[3][0] = -glm::dot(u, position);
    view[3][1] = -glm::dot(v, position);
    view[3][2] = -glm::dot(w, position);
    m_cameraBackend.view = view;
}