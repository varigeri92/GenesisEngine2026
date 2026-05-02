#include "SceneHierarchyWindow.h"

#include <cstdint>
#include <string>
#include <vector>

#include "Genesis.h"
#include "GenesisMaterialIcons.h"
#include "../../EditorSelection.h"

namespace
{
    std::string GetEntityName(entt::registry& registry, gns::entityHandle entity)
    {
        const auto* entityComponent = registry.try_get<EntityComponent>(entity);
        if (entityComponent == nullptr)
        {
            return "Entity";
        }

        return entityComponent->name;
    }

    ImGuiTreeNodeFlags GetTreeFlags(
        gns::entityHandle entity,
        const HierarchyComponent* hierarchy,
        bool isRoot)
    {
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (isRoot)
        {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        if (hierarchy == nullptr || hierarchy->children.empty())
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        if (EditorSelection::IsSelected(entity))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        return flags;
    }

    void DrawEntityNode(entt::registry& registry, gns::entityHandle entity, bool isRoot)
    {
        if (!registry.valid(entity))
        {
            return;
        }

        const auto* hierarchy = registry.try_get<HierarchyComponent>(entity);
        const std::string name = GetEntityName(registry, entity);
        const std::string label = isRoot
            ? std::string(ICON_MD_ACCOUNT_TREE " ") + name + " " ICON_MD_LOCK
            : std::string(ICON_MD_ARTICLE " ") + name;
        const ImGuiTreeNodeFlags flags = GetTreeFlags(entity, hierarchy, isRoot);
        const auto treeId = static_cast<uintptr_t>(entt::to_integral(entity)) + 1u;

        const bool open = ImGui::TreeNodeEx(
            reinterpret_cast<void*>(treeId),
            flags,
            "%s",
            label.c_str());

        if (ImGui::IsItemClicked())
        {
            EditorSelection::SelectEntity(entity);
        }

        if ((flags & ImGuiTreeNodeFlags_NoTreePushOnOpen) != 0)
        {
            return;
        }

        if (open && hierarchy != nullptr)
        {
            const std::vector<gns::entityHandle> children = hierarchy->children;
            for (gns::entityHandle child : children)
            {
                DrawEntityNode(registry, child, false);
            }

            ImGui::TreePop();
        }
    }
}

void SceneHierarchyWindow::OnDraw()
{
    const auto& scenes = gns::SceneManager::GetLoadedScenes();
    if (scenes.empty())
    {
        ImGui::TextDisabled("No loaded scenes");
        return;
    }

    auto& registry = gns::core::SystemsManager::GetRegistry();
    for (const auto& scene : scenes)
    {
        if (scene == nullptr)
        {
            continue;
        }

        DrawEntityNode(registry, scene->root.entity_handle, true);
    }
}
