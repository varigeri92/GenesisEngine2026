#include "TestEditorWindow.h"
#include "GenesisGUI.h"
#include "../Engine/Systems/SystemsManager.h"
#include "EditorGUI/EditorWidgets.h"
#include "Genesis.h"

void TestEditorWindow::OnDraw()
{
    ImGui::Text("Editor window");

    gns::core::SystemsManager::ForEach<EntityComponent, AmbientLightComponent>(
    [&](EntityComponent& entityComp, AmbientLightComponent& ambient_light)
    {
        if (widgets::startWidgets(entityComp.name + "##AmbientLight")){
            widgets::float4_widget("Ambient Color", reinterpret_cast<float*>(&ambient_light.color));
            widgets::endWidgets();
        }
    });

    gns::core::SystemsManager::ForEach<EntityComponent, DirectionalLightComponent>(
    [&](EntityComponent& entityComp, DirectionalLightComponent& directional_light)
    {
        if (widgets::startWidgets(entityComp.name + "##DirectionalLight")){
            widgets::float4_widget("Sun Color", reinterpret_cast<float*>(&directional_light.color));
            widgets::endWidgets();
        }
    });
}
