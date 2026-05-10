#include "InspectorWindow.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

#include "Genesis.h"
#include "../../../Engine/Assets/AssetManager.h"
#include "../../../Engine/Object/Material.h"
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
            auto* entity = static_cast<gns::entityHandle*>(fieldValue);
            uint32_t value = gns::Entity(*entity).GetDebugId();
            if (ImGui::InputScalar("##value", ImGuiDataType_U32, &value))
            {
                *entity = static_cast<gns::entityHandle>(value);
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

    bool ContainsToken(const std::string& value, const char* token)
    {
        std::string lower = value;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });

        return lower.find(token) != std::string::npos;
    }

    bool IsColorProperty(const std::string& name)
    {
        return ContainsToken(name, "color") ||
            ContainsToken(name, "albedo") ||
            ContainsToken(name, "tint");
    }

    const char* MaterialPropertyTypeName(gns::MaterialPropertyType type)
    {
        switch (type)
        {
        case gns::MaterialPropertyType::Float: return "float";
        case gns::MaterialPropertyType::Int: return "int";
        case gns::MaterialPropertyType::UInt: return "uint";
        case gns::MaterialPropertyType::Vec2: return "vec2";
        case gns::MaterialPropertyType::Vec3: return "vec3";
        case gns::MaterialPropertyType::Vec4: return "vec4";
        case gns::MaterialPropertyType::Mat4: return "mat4";
        case gns::MaterialPropertyType::FloatArray: return "float[]";
        case gns::MaterialPropertyType::IntArray: return "int[]";
        case gns::MaterialPropertyType::UIntArray: return "uint[]";
        case gns::MaterialPropertyType::Bytes: return "bytes";
        default: return "unknown";
        }
    }

    bool DrawHandleField(const char* label, gns::Handle& handle)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(label);
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1.0f);

        uint64_t value = handle.Get();
        if (ImGui::InputScalar("##value", ImGuiDataType_U64, &value))
        {
            handle = gns::Handle::Create(value);
            return true;
        }

        return false;
    }

    template<typename T>
    bool ReadMaterialArray(
        const gns::Material& material,
        const gns::MaterialPropertyInfo& property,
        std::vector<T>& values)
    {
        const void* propertyData = material.GetPropertyData(property);
        if (propertyData == nullptr || property.elementSize < sizeof(T))
        {
            return false;
        }

        values.resize(property.elementCount);
        const auto* bytes = static_cast<const uint8_t*>(propertyData);
        for (uint32_t index = 0; index < property.elementCount; ++index)
        {
            std::memcpy(&values[index], bytes + property.elementStride * index, sizeof(T));
        }

        return true;
    }

    template<typename T>
    bool DrawScalarArray(
        const gns::Material& material,
        const gns::MaterialPropertyInfo& property,
        ImGuiDataType dataType,
        std::vector<T>& values)
    {
        if (!ReadMaterialArray(material, property, values))
        {
            ImGui::TextDisabled("Invalid data");
            return false;
        }

        bool changed = false;
        if (ImGui::TreeNodeEx("##array", ImGuiTreeNodeFlags_DefaultOpen, "%u values", property.elementCount))
        {
            const float dragSpeed = dataType == ImGuiDataType_Float ? 0.01f : 1.0f;
            for (uint32_t index = 0; index < property.elementCount; ++index)
            {
                ImGui::PushID(static_cast<int>(index));
                ImGui::Text("[%u]", index);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-1.0f);
                changed |= ImGui::DragScalar("##value", dataType, &values[index], dragSpeed);
                ImGui::PopID();
            }
            ImGui::TreePop();
        }

        return changed;
    }

    void SyncLegacyMaterialFields(gns::Material& material, const std::string& propertyName)
    {
        if (propertyName == "albedo_color")
        {
            material.albedo_color = material.GetColor4(propertyName, material.albedo_color);
        }
    }

    void DrawMaterialPropertyValue(
        gns::Material& material,
        const gns::MaterialPropertyInfo& property)
    {
        const std::string& name = property.name;
        bool changed = false;

        switch (property.type)
        {
        case gns::MaterialPropertyType::Float:
        {
            float value = material.GetFloat(name);
            changed = ImGui::DragFloat("##value", &value, 0.01f);
            if (changed)
            {
                material.SetFloat(name, value);
            }
            break;
        }
        case gns::MaterialPropertyType::Int:
        {
            int32_t value = material.GetInt(name);
            changed = ImGui::DragScalar("##value", ImGuiDataType_S32, &value, 1.0f);
            if (changed)
            {
                material.SetInt(name, value);
            }
            break;
        }
        case gns::MaterialPropertyType::UInt:
        {
            uint32_t value = material.GetUInt(name);
            changed = ImGui::DragScalar("##value", ImGuiDataType_U32, &value, 1.0f);
            if (changed)
            {
                material.SetUInt(name, value);
            }
            break;
        }
        case gns::MaterialPropertyType::Vec2:
        {
            glm::vec2 value = material.GetVec2(name);
            changed = ImGui::DragFloat2("##value", &value.x, 0.01f);
            if (changed)
            {
                material.SetVec2(name, value);
            }
            break;
        }
        case gns::MaterialPropertyType::Vec3:
        {
            glm::vec3 value = material.GetVec3(name);
            if (IsColorProperty(name))
            {
                changed = ImGui::ColorEdit3(
                    "##value",
                    &value.x,
                    ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                if (changed)
                {
                    material.SetColor3(name, value);
                }
            }
            else
            {
                changed = ImGui::DragFloat3("##value", &value.x, 0.01f);
                if (changed)
                {
                    material.SetVec3(name, value);
                }
            }
            break;
        }
        case gns::MaterialPropertyType::Vec4:
        {
            glm::vec4 value = material.GetVec4(name);
            if (IsColorProperty(name))
            {
                changed = ImGui::ColorEdit4(
                    "##value",
                    &value.x,
                    ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                if (changed)
                {
                    material.SetColor4(name, value);
                }
            }
            else
            {
                changed = ImGui::DragFloat4("##value", &value.x, 0.01f);
                if (changed)
                {
                    material.SetVec4(name, value);
                }
            }
            break;
        }
        case gns::MaterialPropertyType::Mat4:
        {
            glm::mat4 value = material.GetMat4(name);
            for (int column = 0; column < 4; ++column)
            {
                ImGui::PushID(column);
                ImGui::SetNextItemWidth(-1.0f);
                changed |= ImGui::DragFloat4("##column", &value[column][0], 0.01f);
                ImGui::PopID();
            }

            if (changed)
            {
                material.SetMat4(name, value);
            }
            break;
        }
        case gns::MaterialPropertyType::FloatArray:
        {
            std::vector<float> values;
            changed = DrawScalarArray(material, property, ImGuiDataType_Float, values);
            if (changed)
            {
                material.SetFloatArray(name, values);
            }
            break;
        }
        case gns::MaterialPropertyType::IntArray:
        {
            std::vector<int32_t> values;
            changed = DrawScalarArray(material, property, ImGuiDataType_S32, values);
            if (changed)
            {
                material.SetIntArray(name, values);
            }
            break;
        }
        case gns::MaterialPropertyType::UIntArray:
        {
            std::vector<uint32_t> values;
            changed = DrawScalarArray(material, property, ImGuiDataType_U32, values);
            if (changed)
            {
                material.SetUIntArray(name, values);
            }
            break;
        }
        case gns::MaterialPropertyType::Bytes:
            ImGui::TextDisabled("%zu bytes", property.size);
            break;
        default:
            ImGui::TextDisabled("Unsupported");
            break;
        }

        if (changed)
        {
            SyncLegacyMaterialFields(material, name);
        }
    }

    void DrawMaterialProperties(gns::Material& material, bool debugView)
    {
        const auto& properties = material.GetProperties();
        if (properties.empty())
        {
            ImGui::TextDisabled("No material properties");
            return;
        }

        constexpr ImGuiTableFlags tableFlags =
            ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_NoPadInnerX |
            ImGuiTableFlags_NoPadOuterX;

        if (ImGui::BeginTable("##materialProperties", 2, tableFlags))
        {
            ImGui::TableSetupColumn("##Property", ImGuiTableColumnFlags_WidthFixed, 130.0f);
            ImGui::TableSetupColumn("##Value", ImGuiTableColumnFlags_WidthStretch);

            for (const gns::MaterialPropertyInfo& property : properties)
            {
                ImGui::PushID(property.name.c_str());
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(property.name.c_str());

                if (debugView)
                {
                    ImGui::TextDisabled(
                        "%s | off %zu | size %zu",
                        MaterialPropertyTypeName(property.type),
                        property.offset,
                        property.size);
                }

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-1.0f);
                DrawMaterialPropertyValue(material, property);
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    void DrawMaterialSection(gns::Material& material, bool debugView)
    {
        ImGui::PushID("SelectedMaterial");
        const bool open = ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen);
        if (!open)
        {
            ImGui::PopID();
            return;
        }

        constexpr ImGuiTableFlags tableFlags =
            ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_NoPadInnerX |
            ImGuiTableFlags_NoPadOuterX;

        if (ImGui::BeginTable("##materialRefs", 2, tableFlags))
        {
            ImGui::TableSetupColumn("##Field", ImGuiTableColumnFlags_WidthFixed, 130.0f);
            ImGui::TableSetupColumn("##Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("name");
            ImGui::TableNextColumn();
            const std::string materialName = material.GetName();
            ImGui::TextUnformatted(materialName.c_str());

            DrawHandleField("shader", material.shader_ref.m_handle);
            DrawHandleField("albedo texture", material.albedo_texture.m_handle);

            ImGui::EndTable();
        }

        ImGui::Separator();
        DrawMaterialProperties(material, debugView);
        ImGui::PopID();
    }

    void DrawSelectedEntityMaterial(
        gns::entityHandle selectedEntity,
        bool debugView)
    {
        MeshComponent* meshComponent = gns::Entity(selectedEntity).TryGetComponent<MeshComponent>();
        if (meshComponent == nullptr || !meshComponent->material.m_handle.IsValid())
        {
            return;
        }

        gns::Material* material = gns::assets::AssetManager::EnsureMaterialLoaded(
            meshComponent->material.m_handle);
        if (material == nullptr)
        {
            return;
        }

        DrawMaterialSection(*material, debugView);
    }
}

void InspectorWindow::OnDraw()
{
    static bool debugView = false;
    ImGui::Checkbox("Debug", &debugView);
    ImGui::Separator();

    const gns::entityHandle selectedEntity = EditorSelection::GetSelectedEntity();
    gns::Entity entity(selectedEntity);
    if (!entity.IsValid())
    {
        ImGui::TextDisabled("No entity selected");
        return;
    }

    ImGui::Text("Entity: %u", entity.GetDebugId());

    const auto& components = gns::reflection::ComponentRegistry::GetComponents();
    for (const gns::reflection::ComponentMeta& component : components)
    {
        if (component.has_component == nullptr ||
            component.get_component == nullptr ||
            !component.has_component(entity))
        {
            continue;
        }

        void* componentValue = component.get_component(entity);
        if (componentValue == nullptr)
        {
            continue;
        }

        DrawComponent(component, componentValue, debugView);
    }

    DrawSelectedEntityMaterial(selectedEntity, debugView);
}
