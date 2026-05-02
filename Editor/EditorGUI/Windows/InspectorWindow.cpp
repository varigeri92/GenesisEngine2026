#include "InspectorWindow.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "Genesis.h"
#include "../../EditorSelection.h"

namespace
{
    void DrawFieldValue(const gns::reflection::FieldMeta& field, void* fieldValue)
    {
        switch (field.kind)
        {
        case gns::reflection::FieldKind::Bool:
            ImGui::Checkbox("##value", static_cast<bool*>(fieldValue));
            break;
        case gns::reflection::FieldKind::Int:
            ImGui::DragInt("##value", static_cast<int*>(fieldValue));
            break;
        case gns::reflection::FieldKind::Float:
            ImGui::DragFloat("##value", static_cast<float*>(fieldValue), 0.01f);
            break;
        case gns::reflection::FieldKind::String:
        {
            auto* value = static_cast<std::string*>(fieldValue);
            std::array<char, 256> buffer = {};
            const size_t copySize = value->size() < buffer.size() - 1
                ? value->size()
                : buffer.size() - 1;
            std::memcpy(buffer.data(), value->data(), copySize);
            if (ImGui::InputText("##value", buffer.data(), buffer.size()))
            {
                *value = buffer.data();
            }
            break;
        }
        case gns::reflection::FieldKind::Vec2:
            ImGui::DragFloat2("##value", reinterpret_cast<float*>(fieldValue), 0.01f);
            break;
        case gns::reflection::FieldKind::Vec3:
            ImGui::DragFloat3("##value", reinterpret_cast<float*>(fieldValue), 0.01f);
            break;
        case gns::reflection::FieldKind::Vec4:
            ImGui::DragFloat4("##value", reinterpret_cast<float*>(fieldValue), 0.01f);
            break;
        case gns::reflection::FieldKind::Handle:
        {
            auto* handle = static_cast<gns::Handle*>(fieldValue);
            uint64_t value = handle->Get();
            if (ImGui::InputScalar("##value", ImGuiDataType_U64, &value))
            {
                *handle = gns::Handle::Create(value);
            }
            break;
        }
        case gns::reflection::FieldKind::EntityHandle:
        {
            auto* entity = static_cast<entt::entity*>(fieldValue);
            auto value = entt::to_integral(*entity);
            if (ImGui::InputScalar("##value", ImGuiDataType_U32, &value))
            {
                *entity = static_cast<entt::entity>(value);
            }
            break;
        }
        case gns::reflection::FieldKind::Reference:
        {
            uint64_t handle = field.get_reference_handle != nullptr
                ? field.get_reference_handle(fieldValue)
                : gns::Handle::Invalid;
            if (field.set_reference_handle != nullptr &&
                ImGui::InputScalar("##value", ImGuiDataType_U64, &handle))
            {
                field.set_reference_handle(fieldValue, handle);
            }

            if (!field.reference_type_name.empty())
            {
                ImGui::TextDisabled("type: %s", field.reference_type_name.c_str());
            }

            if (field.get_reference_type_id != nullptr)
            {
                ImGui::TextDisabled("type id: %zu", field.get_reference_type_id(fieldValue));
            }
            break;
        }
        default:
            ImGui::TextDisabled("Unsupported field");
            break;
        }
    }

    bool HasVisibleFields(const gns::reflection::ComponentMeta& component, bool debugView)
    {
        if (debugView)
        {
            return true;
        }

        for (const gns::reflection::FieldMeta& field : component.fields)
        {
            if (!field.IsHidden())
            {
                return true;
            }
        }

        return false;
    }

    void DrawComponent(
        const gns::reflection::ComponentMeta& component,
        void* componentValue,
        bool debugView)
    {
        if (!HasVisibleFields(component, debugView))
        {
            return;
        }

        ImGui::PushID(component.name.c_str());
        const bool open = ImGui::CollapsingHeader(
            component.name.c_str(),
            ImGuiTreeNodeFlags_DefaultOpen);

        if (open)
        {
            constexpr ImGuiTableFlags tableFlags =
                ImGuiTableFlags_SizingStretchProp |
                ImGuiTableFlags_SizingFixedFit |
                ImGuiTableFlags_NoPadInnerX |
                ImGuiTableFlags_NoPadOuterX;

            if (ImGui::BeginTable("##fields", 2, tableFlags))
            {
                ImGui::TableSetupColumn("##Field", ImGuiTableColumnFlags_WidthFixed, 130.0f);
                ImGui::TableSetupColumn("##Value", ImGuiTableColumnFlags_WidthStretch);

                for (const gns::reflection::FieldMeta& field : component.fields)
                {
                    if (field.IsHidden() && !debugView)
                    {
                        continue;
                    }

                    void* fieldValue = field.get_field != nullptr
                        ? field.get_field(componentValue)
                        : nullptr;
                    if (fieldValue == nullptr)
                    {
                        continue;
                    }

                    ImGui::PushID(field.name.c_str());
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(field.name.c_str());

                    if (debugView)
                    {
                        ImGui::SameLine();
                        ImGui::TextDisabled("(%s)", field.type_name.c_str());
                    }

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-1.0f);

                    if (field.IsEditorReadOnly())
                    {
                        ImGui::BeginDisabled();
                    }

                    DrawFieldValue(field, fieldValue);

                    if (field.IsEditorReadOnly())
                    {
                        ImGui::EndDisabled();
                    }

                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }

        ImGui::PopID();
    }
}

void InspectorWindow::OnDraw()
{
    static bool debugView = false;
    ImGui::Checkbox("Debug", &debugView);
    ImGui::Separator();

    const gns::entityHandle selectedEntity = EditorSelection::GetSelectedEntity();
    auto& registry = gns::core::SystemsManager::GetRegistry();
    if (selectedEntity == entt::null || !registry.valid(selectedEntity))
    {
        ImGui::TextDisabled("No entity selected");
        return;
    }

    ImGui::Text("Entity: %u", entt::to_integral(selectedEntity));

    const auto& components = gns::reflection::ComponentRegistry::GetComponents();
    for (const gns::reflection::ComponentMeta& component : components)
    {
        if (component.has_component == nullptr ||
            component.get_component == nullptr ||
            !component.has_component(registry, selectedEntity))
        {
            continue;
        }

        void* componentValue = component.get_component(registry, selectedEntity);
        if (componentValue == nullptr)
        {
            continue;
        }

        DrawComponent(component, componentValue, debugView);
    }
}
