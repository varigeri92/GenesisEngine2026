#include "EditorSelection.h"

gns::entityHandle EditorSelection::SelectedEntity = entt::null;

void EditorSelection::SelectEntity(gns::entityHandle entity)
{
    SelectedEntity = entity;
}

gns::entityHandle EditorSelection::GetSelectedEntity()
{
    return SelectedEntity;
}

bool EditorSelection::IsSelected(gns::entityHandle entity)
{
    return SelectedEntity == entity;
}
