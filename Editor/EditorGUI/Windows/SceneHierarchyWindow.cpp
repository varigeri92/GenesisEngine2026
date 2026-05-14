#include "SceneHierarchyWindow.h"

#include <cstdint>
#include <string>
#include <vector>

#include "Genesis.h"
#include "GenesisMaterialIcons.h"
#include "../../EditorSelection.h"

namespace
{
    void DrawCreateEntityMenu(gns::Handle sceneHandle, gns::entityHandle parent)
    {
        if (!ImGui::BeginMenu("Add New Entity"))
        {
            return;
        }

        if (ImGui::MenuItem("Empty"))
        {
            gns::Entity entity = gns::Entity::CreateEntity("Empty", sceneHandle, parent);
            if (entity.IsValid())
            {
                EditorSelection::SelectEntity(entity.GetHandle());
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Cube"))
        {
            gns::Entity entity = gns::Entity::CreateEntity("Empty", sceneHandle, parent);
            if (entity.IsValid())
            {
                EditorSelection::SelectEntity(entity.GetHandle());
            }
        }
        if (ImGui::MenuItem("Sphere"))
        {
            gns::Entity entity = gns::Entity::CreateEntity("Empty", sceneHandle, parent);
            if (entity.IsValid())
            {
                EditorSelection::SelectEntity(entity.GetHandle());
            }
        }
        if (ImGui::MenuItem("Plane"))
        {
            gns::Entity entity = gns::Entity::CreateEntity("Empty", sceneHandle, parent);
            if (entity.IsValid())
            {
                EditorSelection::SelectEntity(entity.GetHandle());
            }
        }
        if (ImGui::MenuItem("Quad"))
        {
            gns::Entity entity = gns::Entity::CreateEntity("Empty", sceneHandle, parent);
            if (entity.IsValid())
            {
                EditorSelection::SelectEntity(entity.GetHandle());
            }
        }
        if (ImGui::MenuItem("Capsule"))
        {
            gns::Entity entity = gns::Entity::CreateEntity("Empty", sceneHandle, parent);
            if (entity.IsValid())
            {
                EditorSelection::SelectEntity(entity.GetHandle());
            }
        }
        
        if (ImGui::MenuItem("Knot"))
        {
            gns::Entity entity = gns::Entity::CreateEntity("Empty", sceneHandle, parent);
            if (entity.IsValid())
            {
                EditorSelection::SelectEntity(entity.GetHandle());
            }
        }
        if (ImGui::MenuItem("Torus"))
        {
            gns::Entity entity = gns::Entity::CreateEntity("Empty", sceneHandle, parent);
            if (entity.IsValid())
            {
                EditorSelection::SelectEntity(entity.GetHandle());
            }
        }
        
        ImGui::EndMenu();
    }

    ImGuiTreeNodeFlags GetTreeFlags(
        gns::Entity entity,
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

        if (EditorSelection::IsSelected(entity.GetHandle()))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        return flags;
    }

    void DrawEntityNode(gns::entityHandle entityHandle, bool isRoot)
    {
        const gns::Entity entity(entityHandle);
        if (!entity.IsValid())
        {
            return;
        }

        const auto* hierarchy = entity.TryGetComponent<HierarchyComponent>();
        const std::string& entityName = entity.Name();
        const std::string name = entityName.empty() ? "Entity" : entityName;
        const std::string label = isRoot
            ? std::string(ICON_MD_ACCOUNT_TREE " ") + name + " " ICON_MD_LOCK
            : std::string(ICON_MD_ARTICLE " ") + name;
        const ImGuiTreeNodeFlags flags = GetTreeFlags(entity, hierarchy, isRoot);
        const auto treeId = static_cast<uintptr_t>(entity.GetDebugId()) + 1u;

        const bool open = ImGui::TreeNodeEx(
            reinterpret_cast<void*>(treeId),
            flags,
            "%s",
            label.c_str());

        if (ImGui::IsItemClicked())
        {
            EditorSelection::SelectEntity(entity.GetHandle());
        }

        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight))
        {
            EditorSelection::SelectEntity(entity.GetHandle());
            DrawCreateEntityMenu(entity.SceneHandle(), entity.GetHandle());
            ImGui::EndPopup();
        }

        if ((flags & ImGuiTreeNodeFlags_NoTreePushOnOpen) != 0)
        {
            return;
        }

        if (open && hierarchy != nullptr)
        {
            const std::vector<gns::entityHandle> children = entity.Children();
            for (gns::entityHandle child : children)
            {
                DrawEntityNode(child, false);
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

    for (const auto& scene : scenes)
    {
        if (scene == nullptr)
        {
            continue;
        }

        DrawEntityNode(scene->root.entity_handle, true);
    }
    
    if (ImGui::BeginPopupContextWindow(
        "SceneHierarchyContextMenu",
        ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        gns::Scene& activeScene = gns::SceneManager::GetActiveScene();
        DrawCreateEntityMenu(activeScene.handle, activeScene.root.entity_handle);
        ImGui::EndPopup();
    }
}
