#include "gnspch.h"
#include "SceneSerializer.h"

#include <cstdint>
#include <filesystem>
#include <vector>

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

#include "../Core/ComponentLibrary.h"
#include "../Core/ComponentReflection.h"
#include "../Systems/SystemsManager.h"
#include "../Utils/Path.h"

namespace
{
    constexpr uint32_t SerializerVersion = 1;
    constexpr int64_t NoParentIndex = -1;

    struct SerializedEntity
    {
        gns::entityHandle entity = entt::null;
        int64_t parentIndex = NoParentIndex;
    };

    void EmitVec2(YAML::Emitter& emitter, const glm::vec2& value)
    {
        emitter << YAML::Flow << YAML::BeginSeq << value.x << value.y << YAML::EndSeq;
    }

    void EmitVec3(YAML::Emitter& emitter, const glm::vec3& value)
    {
        emitter << YAML::Flow << YAML::BeginSeq << value.x << value.y << value.z << YAML::EndSeq;
    }

    void EmitVec4(YAML::Emitter& emitter, const glm::vec4& value)
    {
        emitter << YAML::Flow << YAML::BeginSeq << value.x << value.y << value.z << value.w << YAML::EndSeq;
    }

    std::string GetEntityName(entt::registry& registry, gns::entityHandle entity)
    {
        const auto* entityComponent = registry.try_get<EntityComponent>(entity);
        if (entityComponent == nullptr)
        {
            return "Entity";
        }

        return entityComponent->name;
    }

    bool ShouldEmitField(
        const gns::reflection::ComponentMeta& component,
        const gns::reflection::FieldMeta& field)
    {
        if (field.kind == gns::reflection::FieldKind::EntityHandle)
        {
            return false;
        }

        if (component.name == gns::reflection::TypeName<HierarchyComponent>())
        {
            return false;
        }

        return true;
    }

    void EmitReferenceField(
        YAML::Emitter& emitter,
        const gns::reflection::FieldMeta& field,
        void* fieldValue)
    {
        const uint64_t handle = field.get_reference_handle != nullptr
            ? field.get_reference_handle(fieldValue)
            : gns::Handle::Invalid;
        const size_t typeId = field.get_reference_type_id != nullptr
            ? field.get_reference_type_id(fieldValue)
            : 0;

        emitter << YAML::BeginMap;
        emitter << YAML::Key << "handle" << YAML::Value << handle;
        emitter << YAML::Key << "type" << YAML::Value << field.reference_type_name;
        emitter << YAML::Key << "typeId" << YAML::Value << typeId;
        emitter << YAML::EndMap;
    }

    void EmitFieldValue(
        YAML::Emitter& emitter,
        const gns::reflection::FieldMeta& field,
        void* fieldValue)
    {
        switch (field.kind)
        {
        case gns::reflection::FieldKind::Bool:
            emitter << *static_cast<bool*>(fieldValue);
            break;
        case gns::reflection::FieldKind::Int:
            emitter << *static_cast<int*>(fieldValue);
            break;
        case gns::reflection::FieldKind::Float:
            emitter << *static_cast<float*>(fieldValue);
            break;
        case gns::reflection::FieldKind::String:
            emitter << *static_cast<std::string*>(fieldValue);
            break;
        case gns::reflection::FieldKind::Vec2:
            EmitVec2(emitter, *static_cast<glm::vec2*>(fieldValue));
            break;
        case gns::reflection::FieldKind::Vec3:
            EmitVec3(emitter, *static_cast<glm::vec3*>(fieldValue));
            break;
        case gns::reflection::FieldKind::Vec4:
            EmitVec4(emitter, *static_cast<glm::vec4*>(fieldValue));
            break;
        case gns::reflection::FieldKind::Handle:
            emitter << static_cast<gns::Handle*>(fieldValue)->Get();
            break;
        case gns::reflection::FieldKind::EntityHandle:
            emitter << YAML::Null;
            break;
        case gns::reflection::FieldKind::Reference:
            EmitReferenceField(emitter, field, fieldValue);
            break;
        default:
            emitter << YAML::Null;
            break;
        }
    }

    void EmitComponentFields(
        YAML::Emitter& emitter,
        const gns::reflection::ComponentMeta& component,
        void* componentValue)
    {
        emitter << YAML::BeginMap;

        for (const gns::reflection::FieldMeta& field : component.fields)
        {
            if (!ShouldEmitField(component, field))
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

            emitter << YAML::Key << field.name << YAML::Value;
            EmitFieldValue(emitter, field, fieldValue);
        }

        emitter << YAML::EndMap;
    }

    void EmitEntityComponents(
        YAML::Emitter& emitter,
        entt::registry& registry,
        gns::entityHandle entity)
    {
        emitter << YAML::BeginMap;

        const auto& components = gns::reflection::ComponentRegistry::GetComponents();
        for (const gns::reflection::ComponentMeta& component : components)
        {
            if (component.has_component == nullptr ||
                component.get_component == nullptr ||
                !component.has_component(registry, entity))
            {
                continue;
            }

            void* componentValue = component.get_component(registry, entity);
            if (componentValue == nullptr)
            {
                continue;
            }

            emitter << YAML::Key << component.name << YAML::Value;
            EmitComponentFields(emitter, component, componentValue);
        }

        emitter << YAML::EndMap;
    }

    void EmitEntity(
        YAML::Emitter& emitter,
        entt::registry& registry,
        const SerializedEntity& serializedEntity)
    {
        const gns::entityHandle entity = serializedEntity.entity;

        emitter << YAML::BeginMap;
        emitter << YAML::Key << "name" << YAML::Value << GetEntityName(registry, entity);
        emitter << YAML::Key << "parent" << YAML::Value << serializedEntity.parentIndex;
        emitter << YAML::Key << "components" << YAML::Value;
        EmitEntityComponents(emitter, registry, entity);
        emitter << YAML::EndMap;
    }

    std::vector<SerializedEntity> CollectEntities(entt::registry& registry, gns::entityHandle root)
    {
        std::vector<SerializedEntity> entities;
        if (!registry.valid(root))
        {
            return entities;
        }

        std::vector<SerializedEntity> stack;
        stack.push_back({ root, NoParentIndex });

        while (!stack.empty())
        {
            const SerializedEntity current = stack.back();
            stack.pop_back();

            if (!registry.valid(current.entity))
            {
                continue;
            }

            const int64_t currentIndex = static_cast<int64_t>(entities.size());
            entities.push_back(current);

            const auto* hierarchy = registry.try_get<HierarchyComponent>(current.entity);
            if (hierarchy == nullptr || hierarchy->children.empty())
            {
                continue;
            }

            for (auto it = hierarchy->children.rbegin(); it != hierarchy->children.rend(); ++it)
            {
                if (registry.valid(*it))
                {
                    stack.push_back({ *it, currentIndex });
                }
            }
        }

        return entities;
    }

    void EmitEntities(
        YAML::Emitter& emitter,
        entt::registry& registry,
        const std::vector<SerializedEntity>& entities)
    {
        emitter << YAML::BeginSeq;
        for (const SerializedEntity& entity : entities)
        {
            EmitEntity(emitter, registry, entity);
        }
        emitter << YAML::EndSeq;
    }
}

std::filesystem::path gns::SceneSerializer::GetSceneSavePath(const Scene& scene)
{
    return path::Resolve(path::Root::ProjectAssets, scene.name + ".gnsscene");
}

bool gns::SceneSerializer::SaveScene(const Scene& scene)
{
    auto& registry = core::SystemsManager::GetRegistry();
    if (!registry.valid(scene.root.entity_handle))
    {
        LOG_ERROR("[SceneSerializer]: Cannot save scene with invalid root entity.");
        LOG_ERROR(scene.name);
        return false;
    }

    const std::vector<SerializedEntity> entities = CollectEntities(registry, scene.root.entity_handle);

    YAML::Emitter emitter;
    emitter << YAML::BeginMap;
    emitter << YAML::Key << "scene" << YAML::Value << scene.name;
    emitter << YAML::Key << "serializerVersion" << YAML::Value << SerializerVersion;
    emitter << YAML::Key << "entities" << YAML::Value;
    EmitEntities(emitter, registry, entities);
    emitter << YAML::EndMap;

    if (!emitter.good())
    {
        LOG_ERROR("[SceneSerializer]: Failed to create scene YAML.");
        LOG_ERROR(scene.name);
        return false;
    }

    const std::filesystem::path savePath = GetSceneSavePath(scene);
    if (!path::WriteTextFile(savePath, emitter.c_str()))
    {
        LOG_ERROR("[SceneSerializer]: Failed to write scene file.");
        LOG_ERROR(savePath.string());
        return false;
    }

    LOG_INFO("[SceneSerializer]: Saved scene.");
    LOG_INFO(savePath.string());
    return true;
}
