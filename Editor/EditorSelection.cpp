#include "EditorSelection.h"

gns::entityHandle EditorSelection::SelectedEntity = entt::null;
std::filesystem::path EditorSelection::SelectedFile = {};
EditorSelection::Type EditorSelection::SelectionType = EditorSelection::Type::None;

void EditorSelection::SelectEntity(gns::entityHandle entity)
{
    SelectedEntity = entity;
    SelectedFile.clear();
    SelectionType = entity == entt::null ? Type::None : Type::Entity;
}

gns::entityHandle EditorSelection::GetSelectedEntity()
{
    return SelectedEntity;
}

bool EditorSelection::IsSelected(gns::entityHandle entity)
{
    return SelectionType == Type::Entity && SelectedEntity == entity;
}

void EditorSelection::SelectFile(const std::filesystem::path& filePath)
{
    SelectedEntity = entt::null;
    SelectedFile = filePath.lexically_normal();
    SelectionType = SelectedFile.empty() ? Type::None : Type::File;
}

const std::filesystem::path& EditorSelection::GetSelectedFile()
{
    return SelectedFile;
}

bool EditorSelection::IsFileSelected(const std::filesystem::path& filePath)
{
    return SelectionType == Type::File && SelectedFile == filePath.lexically_normal();
}

EditorSelection::Type EditorSelection::GetSelectionType()
{
    return SelectionType;
}

void EditorSelection::Clear()
{
    SelectedEntity = entt::null;
    SelectedFile.clear();
    SelectionType = Type::None;
}
