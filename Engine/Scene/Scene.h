#pragma once
#include <array>
#include <cstdint>
#include <string>

#include <glm/glm.hpp>
#include "../Core/Entity.h"
#include "../Core/Handles.h"

namespace gns
{
    constexpr uint32_t MaxSceneLights = 128;

    struct SceneData
    {
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 proj = glm::mat4(1.0f);
        glm::mat4 viewproj = glm::mat4(1.0f);
        glm::vec4 ambientColor = {0.03f, 0.03f, 0.03f, 1.0f}; // w for intensity
    };

    struct alignas(16) DirectionalLightGpu
    {
        glm::vec4 direction = {0.0f, -1.0f, 0.0f, 1.0f}; // w for intensity
        glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    };

    struct alignas(16) PointLightGpu
    {
        glm::vec4 position = {0.0f, 0.0f, 0.0f, 10.0f}; // w for range
        glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // w for intensity
    };

    struct alignas(16) SpotLightGpu
    {
        glm::vec4 position = {0.0f, 0.0f, 0.0f, 10.0f}; // w for range
        glm::vec4 direction = {0.0f, -1.0f, 0.0f, 0.0f};
        glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // w for intensity
        glm::vec4 cone = {0.9659258f, 0.8660254f, 0.0f, 0.0f}; // x/y are cos(inner/outer)
    };

    struct alignas(16) DirectionalLightBuffer
    {
        uint32_t count = 0;
        uint32_t padding[3] = {};
        std::array<DirectionalLightGpu, MaxSceneLights> lights = {};
    };

    struct alignas(16) PointLightBuffer
    {
        uint32_t count = 0;
        uint32_t padding[3] = {};
        std::array<PointLightGpu, MaxSceneLights> lights = {};
    };

    struct alignas(16) SpotLightBuffer
    {
        uint32_t count = 0;
        uint32_t padding[3] = {};
        std::array<SpotLightGpu, MaxSceneLights> lights = {};
    };

    struct Scene
    {
        Handle handle;
        std::string name;
        Entity root;
    };
}
