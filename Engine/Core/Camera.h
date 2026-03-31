#pragma once
#include <glm/glm.hpp>

struct CameraBackend
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewProjection;
};

struct Camera
{
    float fov;
    float near;
    float far;
    float aspect;
};
