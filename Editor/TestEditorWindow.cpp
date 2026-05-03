#include "TestEditorWindow.h"
#include "GenesisGUI.h"
#include "../Engine/Systems/SystemsManager.h"
#include "EditorGUI/EditorWidgets.h"
#include "Genesis.h"
#include "entt/entt.hpp"

void TestEditorWindow::OnDraw()
{
    ImGui::Text("Editor window");

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
