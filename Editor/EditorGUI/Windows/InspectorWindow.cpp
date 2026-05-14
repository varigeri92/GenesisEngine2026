#include "InspectorWindow.h"

#include <array>
#include <cstdint>
#include <cstring>

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
        case gns::MaterialPropertyType::Color3: return "color3";
        case gns::MaterialPropertyType::Color4: return "color4";
        case gns::MaterialPropertyType::Mat4: return "mat4";
        case gns::MaterialPropertyType::FloatArray: return "float[]";
        case gns::MaterialPropertyType::IntArray: return "int[]";
        case gns::MaterialPropertyType::UIntArray: return "uint[]";
        case gns::MaterialPropertyType::Texture2D: return "texture2D";
        case gns::MaterialPropertyType::Bytes: return "bytes";
        default: return "unknown";
        }
    }

    template<typename T>
    T* GetMaterialValuePtr(gns::Material& material, const gns::MaterialPropertyInfo& property)
    {
        if (property.elementCount != 1 || property.elementSize < sizeof(T))
        {
            return nullptr;
        }

        return static_cast<T*>(material.GetMutablePropertyData(property));
    }

    template<typename T>
    bool DrawScalarArray(
        gns::Material& material,
        const gns::MaterialPropertyInfo& property,
        ImGuiDataType dataType)
    {
        void* propertyData = material.GetMutablePropertyData(property);
        if (propertyData == nullptr || property.elementSize < sizeof(T))
        {
            ImGui::TextDisabled("Invalid data");
            return false;
        }

        auto* bytes = static_cast<uint8_t*>(propertyData);
        const size_t stride = property.elementStride != 0 ? property.elementStride : sizeof(T);
        bool changed = false;
        if (ImGui::TreeNodeEx("##array", ImGuiTreeNodeFlags_DefaultOpen, "%u values", property.elementCount))
        {
            const float dragSpeed = dataType == ImGuiDataType_Float ? 0.01f : 1.0f;
            for (uint32_t index = 0; index < property.elementCount; ++index)
            {
                T* value = reinterpret_cast<T*>(bytes + stride * index);
                ImGui::PushID(static_cast<int>(index));
                ImGui::Text("[%u]", index);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-1.0f);
                changed |= ImGui::DragScalar("##value", dataType, value, dragSpeed);
                ImGui::PopID();
            }
            ImGui::TreePop();
        }

        return changed;
    }

    void DrawMaterialPropertyValue(
        gns::Material& material,
        const gns::MaterialPropertyInfo& property)
    {
        const std::string& name = property.name;

        switch (property.type)
        {
        case gns::MaterialPropertyType::Float:
        {
            float* value = GetMaterialValuePtr<float>(material, property);
            if (value == nullptr)
            {
                ImGui::TextDisabled("Invalid data");
                break;
            }
            ImGui::DragFloat("##value", value, 0.01f);
            break;
        }
        case gns::MaterialPropertyType::Int:
        {
            int32_t* value = GetMaterialValuePtr<int32_t>(material, property);
            if (value == nullptr)
            {
                ImGui::TextDisabled("Invalid data");
                break;
            }
            ImGui::DragScalar("##value", ImGuiDataType_S32, value, 1.0f);
            break;
        }
        case gns::MaterialPropertyType::UInt:
        {
            uint32_t* value = GetMaterialValuePtr<uint32_t>(material, property);
            if (value == nullptr)
            {
                ImGui::TextDisabled("Invalid data");
                break;
            }
            ImGui::DragScalar("##value", ImGuiDataType_U32, value, 1.0f);
            break;
        }
        case gns::MaterialPropertyType::Vec2:
        {
            glm::vec2* value = GetMaterialValuePtr<glm::vec2>(material, property);
            if (value == nullptr)
            {
                ImGui::TextDisabled("Invalid data");
                break;
            }
            ImGui::DragFloat2("##value", &value->x, 0.01f);
            break;
        }
        case gns::MaterialPropertyType::Vec3:
        {
            glm::vec3* value = GetMaterialValuePtr<glm::vec3>(material, property);
            if (value == nullptr)
            {
                ImGui::TextDisabled("Invalid data");
                break;
            }
            ImGui::DragFloat3("##value", &value->x, 0.01f);
            break;
        }
        case gns::MaterialPropertyType::Vec4:
        {
            glm::vec4* value = GetMaterialValuePtr<glm::vec4>(material, property);
            if (value == nullptr)
            {
                ImGui::TextDisabled("Invalid data");
                break;
            }
            ImGui::DragFloat4("##value", &value->x, 0.01f);
            break;
        }
        case gns::MaterialPropertyType::Color3:
        {
            glm::vec3* value = GetMaterialValuePtr<glm::vec3>(material, property);
            if (value == nullptr)
            {
                ImGui::TextDisabled("Invalid data");
                break;
            }
            ImGui::ColorEdit3(
                "##value",
                &value->x,
                ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
            break;
        }
        case gns::MaterialPropertyType::Color4:
        {
            glm::vec4* value = GetMaterialValuePtr<glm::vec4>(material, property);
            if (value == nullptr)
            {
                ImGui::TextDisabled("Invalid data");
                break;
            }
            ImGui::ColorEdit4(
                "##value",
                &value->x,
                ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
            break;
        }
        case gns::MaterialPropertyType::Mat4:
        {
            glm::mat4* value = GetMaterialValuePtr<glm::mat4>(material, property);
            if (value == nullptr)
            {
                ImGui::TextDisabled("Invalid data");
                break;
            }

            for (int column = 0; column < 4; ++column)
            {
                ImGui::PushID(column);
                ImGui::SetNextItemWidth(-1.0f);
                ImGui::DragFloat4("##column", &(*value)[column][0], 0.01f);
                ImGui::PopID();
            }
            break;
        }
        case gns::MaterialPropertyType::FloatArray:
        {
            DrawScalarArray<float>(material, property, ImGuiDataType_Float);
            break;
        }
        case gns::MaterialPropertyType::IntArray:
        {
            DrawScalarArray<int32_t>(material, property, ImGuiDataType_S32);
            break;
        }
        case gns::MaterialPropertyType::UIntArray:
        {
            DrawScalarArray<uint32_t>(material, property, ImGuiDataType_U32);
            break;
        }
        case gns::MaterialPropertyType::Texture2D:
        {
            gns::Reference<gns::Texture> texture = material.GetTexture(name);
            uint64_t handle = texture.m_handle.Get();
            const bool changed = ImGui::InputScalar("##value", ImGuiDataType_U64, &handle);
            if (changed)
            {
                material.SetTexture(name, gns::Handle::Create(handle));
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
                    if (property.set != gns::InvalidMaterialBinding)
                    {
                        ImGui::TextDisabled(
                            "%s | set %u | binding %u | count %u | off %zu | size %zu",
                            MaterialPropertyTypeName(property.type),
                            property.set,
                            property.binding,
                            property.descriptorCount,
                            property.offset,
                            property.size);
                    }
                    else
                    {
                        ImGui::TextDisabled(
                            "%s | off %zu | size %zu",
                            MaterialPropertyTypeName(property.type),
                            property.offset,
                            property.size);
                    }
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

    void DrawAddComponentMenu(gns::Entity entity)
    {
        if (!entity.IsValid())
        {
            return;
        }

        ImGui::Separator();
        if (ImGui::Button("+ Add Component"))
        {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (!ImGui::BeginPopup("AddComponentPopup"))
        {
            return;
        }

        bool hasAvailableComponent = false;
        const auto& components = gns::reflection::ComponentRegistry::GetComponents();
        for (const gns::reflection::ComponentMeta& component : components)
        {
            if (component.type_id == gns::Handle::CreateFromString(
                gns::reflection::TypeName<SceneRootComponent>()))
            {
                continue;
            }

            if (component.has_component == nullptr ||
                component.ensure_component == nullptr)
            {
                continue;
            }

            const bool alreadyHasComponent = component.has_component(entity);
            hasAvailableComponent |= !alreadyHasComponent;

            if (alreadyHasComponent)
            {
                ImGui::BeginDisabled();
            }

            if (ImGui::MenuItem(component.name.c_str(), alreadyHasComponent ? "Added" : nullptr, false, !alreadyHasComponent))
            {
                component.ensure_component(entity);
                ImGui::CloseCurrentPopup();
            }

            if (alreadyHasComponent)
            {
                ImGui::EndDisabled();
            }
        }

        if (!hasAvailableComponent)
        {
            ImGui::TextDisabled("All reflected components added");
        }

        ImGui::EndPopup();
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

    DrawAddComponentMenu(entity);
    DrawSelectedEntityMaterial(selectedEntity, debugView);
}
