#include "gnspch.h"
#include "TransformHelper.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp> // (optional) but useful
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>


inline float RadiansToDegrees(float radians)
{
    return glm::degrees(radians);
}

inline float DegreesToRadians(float degrees)
{
    return glm::radians(degrees);
}

glm::vec3 gns::TransformHelper::Forward(Transform& transform)
{
    const float pitch = DegreesToRadians(transform.rotation.x);
    const float yaw = DegreesToRadians(transform.rotation.y);
    glm::vec3 forward = {
        cosf(pitch) * sinf(yaw),
        sinf(pitch),
        cosf(pitch) * cosf(yaw)
    };
    return glm::normalize(forward);
}

glm::vec3 gns::TransformHelper::Up(Transform& transform)
{
    const glm::vec3 right = Right(transform);
    const glm::vec3 fwd = Forward(transform);
    return  glm::normalize(glm::cross(right, fwd));
}

glm::vec3 gns::TransformHelper::Right(Transform& transform)
{
    const glm::vec3 fwd = Forward(transform);
   return glm::normalize(glm::cross(fwd, WorldUp));
}
