#include "TestEditorWindow.h"
#include "EditorCameraSystem.h"
#include "GenesisGUI.h"
#include "../Engine/Systems/SystemsManager.h"
#include "EditorGUI/EditorWidgets.h"
#include "Genesis.h"
#include "entt/entt.hpp"

float testFloat[] = {0, 1, 2, 3};

void TestEditorWindow::OnDraw()
{
    EditorCameraSystem* editor_camera_system = gns::core::SystemsManager::GetSystem<EditorCameraSystem>();
    ImGui::Text("Editor window");
    ImGui::DragFloat("fov", &editor_camera_system->m_camera.fov, 0.001f);
    ImGui::DragFloat("near", &editor_camera_system->m_camera.near, 0.001f);
    ImGui::DragFloat("far", &editor_camera_system->m_camera.far, 0.001f);
    ImGui::DragFloat3("pos", reinterpret_cast<float*>(&editor_camera_system->m_position), 0.001f);
    ImGui::DragFloat3("rot", reinterpret_cast<float*>(&editor_camera_system->m_rotation), 0.001f);
    if (ImGui::Button("toggle camera logic"))
    {
        editor_camera_system->test = !editor_camera_system->test;
    }

    auto ambientLights = gns::core::SystemsManager::GetRegistry()
    .view<EntityComponent, AmbientLightComponent>();
    ambientLights.each([&](EntityComponent& entityComp, AmbientLightComponent& ambient_light)
    {
        if (widgets::startWidgets(entityComp.name + "##AmbientLight")){
            widgets::float4_widget("Ambient Color", reinterpret_cast<float*>(&ambient_light.color));
            widgets::endWidgets();
        }
    });

    auto directionalLights = gns::core::SystemsManager::GetRegistry()
    .view<EntityComponent, DirectionalLightComponent>();
    directionalLights.each([&](EntityComponent& entityComp, DirectionalLightComponent& directional_light)
    {
        if (widgets::startWidgets(entityComp.name + "##DirectionalLight")){
            widgets::float4_widget("Sun Direction", reinterpret_cast<float*>(&directional_light.direction));
            widgets::float4_widget("Sun Color", reinterpret_cast<float*>(&directional_light.color));
            widgets::endWidgets();
        }
    });
}
