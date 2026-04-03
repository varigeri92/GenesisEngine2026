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

    
    auto SceneData = gns::core::SystemsManager::GetRegistry()
    .view<EntityComponent, Transform, gns::SceneData>();
    SceneData.each([&](EntityComponent& entityComp, Transform& transform, gns::SceneData& scene_data)
    {
        if (widgets::startWidgets("test")){
            widgets::float4_widget("Ambient Color", reinterpret_cast<float*>(&scene_data.ambientColor));
            widgets::float4_widget("Sun Direction", reinterpret_cast<float*>(&scene_data.sunlightDirection));
            widgets::float4_widget("sun Color", reinterpret_cast<float*>(&scene_data.sunlightColor));
            widgets::endWidgets();
        }
        
    });
    
    
}
