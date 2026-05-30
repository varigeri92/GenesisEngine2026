#include "ModelImportController.h"

#include <imgui.h>

#include "AssetMetadataWriter.h"
#include "../../Engine/Scene/SceneAssetImporter.h"
#include "../../Engine/Utils/Path.h"

namespace
{
    constexpr const char* ModelImportPopupTitle = "Import Model";
}

void editor::assets::ModelImportController::Begin(const std::filesystem::path& assetPath)
{
    m_pendingModelImportPath = gns::path::Normalize(assetPath);
    m_relativePath = assetPath;
    m_loadOptions = {};
    m_error.clear();
    m_popupOpen = true;
    m_shouldOpenPopup = true;
}

void editor::assets::ModelImportController::DrawPopup()
{
    if (!m_popupOpen && !m_shouldOpenPopup)
    {
        return;
    }

    if (m_shouldOpenPopup)
    {
        ImGui::OpenPopup(ModelImportPopupTitle);
        m_shouldOpenPopup = false;
    }

    bool popupOpen = m_popupOpen;
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
    if (ImGui::BeginPopupModal(ModelImportPopupTitle, &popupOpen))
    {
        ImGui::TextWrapped("%s", m_relativePath.string().c_str());
        ImGui::Separator();

        ImGui::Checkbox("Flatten hierarchy", &m_loadOptions.flattenHierarchy);
        ImGui::Checkbox("Import skeleton", &m_loadOptions.importSkeleton);
        ImGui::Checkbox("Import materials", &m_loadOptions.importMaterials);
        ImGui::Checkbox("Import textures", &m_loadOptions.importTextures);

        if (!m_error.empty())
        {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.25f, 1.0f), "%s", m_error.c_str());
        }

        ImGui::Separator();
        if (ImGui::Button("Import", ImVec2(100, 0)))
        {
            if (Complete())
            {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 100.0f);
        if (ImGui::Button("Cancel", ImVec2(100, 0)))
        {
            Cancel();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    if (m_popupOpen && !popupOpen)
    {
        Cancel();
    }
}

void editor::assets::ModelImportController::Cancel()
{
    m_pendingModelImportPath.clear();
    m_loadOptions = {};
    m_error.clear();
    m_popupOpen = false;
    m_shouldOpenPopup = false;
}

bool editor::assets::ModelImportController::Complete()
{
    if (m_pendingModelImportPath.empty())
    {
        m_error = "No model is pending import.";
        return false;
    }

    if (!WriteModelMetaFile(m_pendingModelImportPath, m_loadOptions))
    {
        m_error = "Failed to write model meta file.";
        return false;
    }

    if (!gns::SceneAssetImporter::RequestMeshAssetIntoScene(
        m_pendingModelImportPath,
        m_loadOptions).IsValid())
    {
        m_error = "Model import request failed.";
        return false;
    }

    Cancel();
    return true;
}

bool editor::assets::ModelImportController::LoadModel(std::filesystem::path& metaPath)
{
    ReadMetadataFromFile(metaPath, m_loadOptions);

    if (!gns::SceneAssetImporter::RequestMeshAssetIntoScene(
        m_pendingModelImportPath,
        m_loadOptions).IsValid())
    {
        m_error = "Model import request failed.";
        return false;
    }

    m_shouldOpenPopup = false;
    return true;
}
