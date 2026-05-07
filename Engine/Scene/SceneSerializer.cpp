#include "gnspch.h"
#include "SceneSerializer.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

#include "../Core/ComponentLibrary.h"
#include "../Core/ComponentReflection.h"
#include "../Assets/AssetManager.h"
#include "../Object/Material.h"
#include "../Object/Mesh.h"
#include "../Object/Texture.h"
#include "../Systems/SystemsManager.h"
#include "../Utils/Path.h"
#include "SceneManager.h"

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

    std::filesystem::path ResolveSceneLoadPath(const std::filesystem::path& scenePath)
    {
        if (gns::path::IsAbsolute(scenePath))
        {
            return gns::path::Normalize(scenePath);
        }

        return gns::path::Resolve(gns::path::Root::ProjectAssets, scenePath);
    }

    bool ReadVec2(const YAML::Node& node, glm::vec2& value)
    {
        if (!node.IsSequence() || node.size() != 2)
        {
            return false;
        }

        value = glm::vec2(node[0].as<float>(), node[1].as<float>());
        return true;
    }

    bool ReadVec3(const YAML::Node& node, glm::vec3& value)
    {
        if (!node.IsSequence() || node.size() != 3)
        {
            return false;
        }

        value = glm::vec3(node[0].as<float>(), node[1].as<float>(), node[2].as<float>());
        return true;
    }

    bool ReadVec4(const YAML::Node& node, glm::vec4& value)
    {
        if (!node.IsSequence() || node.size() != 4)
        {
            return false;
        }

        value = glm::vec4(node[0].as<float>(), node[1].as<float>(), node[2].as<float>(), node[3].as<float>());
        return true;
    }

    void EnsureReferencedAsset(const std::string& referenceTypeName, gns::Handle handle)
    {
        if (!handle.IsValid())
        {
            return;
        }

        if (referenceTypeName == gns::reflection::TypeName<gns::Mesh>())
        {
            gns::assets::AssetManager::EnsureMeshLoaded(handle);
        }
        else if (referenceTypeName == gns::reflection::TypeName<gns::Material>())
        {
            gns::assets::AssetManager::EnsureMaterialLoaded(handle);
        }
        else if (referenceTypeName == gns::reflection::TypeName<gns::Texture>())
        {
            gns::assets::AssetManager::EnsureTextureLoaded(handle);
        }
    }

    bool RestoreFieldValue(
        const gns::reflection::FieldMeta& field,
        void* componentValue,
        const YAML::Node& fieldNode)
    {
        if (field.set_field == nullptr)
        {
            return false;
        }

        switch (field.kind)
        {
        case gns::reflection::FieldKind::Bool:
        {
            const bool value = fieldNode.as<bool>();
            field.set_field(componentValue, &value);
            return true;
        }
        case gns::reflection::FieldKind::Int:
        {
            const int value = fieldNode.as<int>();
            field.set_field(componentValue, &value);
            return true;
        }
        case gns::reflection::FieldKind::Float:
        {
            const float value = fieldNode.as<float>();
            field.set_field(componentValue, &value);
            return true;
        }
        case gns::reflection::FieldKind::String:
        {
            const std::string value = fieldNode.as<std::string>();
            field.set_field(componentValue, &value);
            return true;
        }
        case gns::reflection::FieldKind::Vec2:
        {
            glm::vec2 value{};
            if (!ReadVec2(fieldNode, value))
            {
                return false;
            }
            field.set_field(componentValue, &value);
            return true;
        }
        case gns::reflection::FieldKind::Vec3:
        {
            glm::vec3 value{};
            if (!ReadVec3(fieldNode, value))
            {
                return false;
            }
            field.set_field(componentValue, &value);
            return true;
        }
        case gns::reflection::FieldKind::Vec4:
        {
            glm::vec4 value{};
            if (!ReadVec4(fieldNode, value))
            {
                return false;
            }
            field.set_field(componentValue, &value);
            return true;
        }
        case gns::reflection::FieldKind::Handle:
        {
            const gns::Handle value = gns::Handle::Create(fieldNode.as<uint64_t>());
            field.set_field(componentValue, &value);
            return true;
        }
        case gns::reflection::FieldKind::EntityHandle:
            return false;
        case gns::reflection::FieldKind::Reference:
        {
            if (!fieldNode.IsMap() || !fieldNode["handle"] || field.set_reference_handle == nullptr)
            {
                return false;
            }

            void* fieldValue = field.get_field != nullptr ? field.get_field(componentValue) : nullptr;
            if (fieldValue == nullptr)
            {
                return false;
            }

            const gns::Handle value = gns::Handle::Create(fieldNode["handle"].as<uint64_t>());
            field.set_reference_handle(fieldValue, value.Get());
            EnsureReferencedAsset(field.reference_type_name, value);
            return true;
        }
        default:
            return false;
        }
    }

    const gns::reflection::FieldMeta* FindField(
        const gns::reflection::ComponentMeta& component,
        const std::string& fieldName)
    {
        const auto field = std::find_if(
            component.fields.begin(),
            component.fields.end(),
            [&](const gns::reflection::FieldMeta& candidate)
            {
                return candidate.name == fieldName;
            });

        return field != component.fields.end() ? &(*field) : nullptr;
    }

    void RestoreRuntimeSceneComponents(
        entt::registry& registry,
        gns::Scene& scene,
        gns::entityHandle entity,
        bool isRoot)
    {
        if (auto* entityComponent = registry.try_get<EntityComponent>(entity))
        {
            entityComponent->entity_handle = entity;
        }

        if (isRoot)
        {
            auto& rootComponent = registry.get_or_emplace<SceneRootComponent>(entity);
            rootComponent.scene_handle = scene.handle;
        }

        auto& memberComponent = registry.get_or_emplace<SceneMemberComponent>(entity);
        memberComponent.scene_handle = scene.handle;
        (void)registry.get_or_emplace<HierarchyComponent>(entity);
    }

    void RestoreComponentFields(
        entt::registry& registry,
        gns::Scene& scene,
        gns::entityHandle entity,
        const std::string& componentName,
        const YAML::Node& componentNode,
        bool isRoot)
    {
        if (componentName == gns::reflection::TypeName<HierarchyComponent>())
        {
            return;
        }

        if (componentName == gns::reflection::TypeName<SceneRootComponent>())
        {
            if (!isRoot)
            {
                LOG_WARNING("[SceneSerializer]: Ignoring SceneRootComponent on non-root entity.");
            }
            RestoreRuntimeSceneComponents(registry, scene, entity, isRoot);
            return;
        }

        if (componentName == gns::reflection::TypeName<SceneMemberComponent>())
        {
            RestoreRuntimeSceneComponents(registry, scene, entity, isRoot);
            return;
        }

        const gns::reflection::ComponentMeta* component =
            gns::reflection::ComponentRegistry::GetComponentMeta(componentName);
        if (component == nullptr || component->ensure_component == nullptr)
        {
            LOG_WARNING("[SceneSerializer]: Unknown component in scene file.");
            LOG_WARNING(componentName);
            return;
        }

        void* componentValue = component->ensure_component(registry, entity);
        if (componentValue == nullptr)
        {
            LOG_WARNING("[SceneSerializer]: Failed to create component while loading scene.");
            LOG_WARNING(componentName);
            return;
        }

        if (!componentNode.IsMap())
        {
            LOG_WARNING("[SceneSerializer]: Component data is not a map.");
            LOG_WARNING(componentName);
            return;
        }

        for (const auto& fieldNode : componentNode)
        {
            const std::string fieldName = fieldNode.first.as<std::string>();
            const gns::reflection::FieldMeta* field = FindField(*component, fieldName);
            if (field == nullptr)
            {
                LOG_WARNING("[SceneSerializer]: Unknown field in scene file.");
                LOG_WARNING(componentName + "." + fieldName);
                continue;
            }

            try
            {
                if (!RestoreFieldValue(*field, componentValue, fieldNode.second))
                {
                    LOG_WARNING("[SceneSerializer]: Failed to restore field from scene file.");
                    LOG_WARNING(componentName + "." + fieldName);
                }
            }
            catch (const YAML::Exception& exception)
            {
                LOG_WARNING("[SceneSerializer]: Invalid field value in scene file.");
                LOG_WARNING(componentName + "." + fieldName);
                LOG_WARNING(exception.what());
            }
        }

        if (componentName == gns::reflection::TypeName<EntityComponent>())
        {
            static_cast<EntityComponent*>(componentValue)->entity_handle = entity;
        }

        RestoreRuntimeSceneComponents(registry, scene, entity, isRoot);
    }

    std::string ReadEntityName(const YAML::Node& entityNode, const std::string& fallback)
    {
        try
        {
            if (entityNode["name"])
            {
                return entityNode["name"].as<std::string>();
            }
        }
        catch (const YAML::Exception&)
        {
        }

        return fallback;
    }

    void RestoreParents(
        const YAML::Node& entitiesNode,
        std::vector<gns::Entity>& entities,
        gns::Scene& scene)
    {
        for (size_t index = 1; index < entities.size(); ++index)
        {
            int64_t parentIndex = NoParentIndex;
            try
            {
                if (entitiesNode[index]["parent"])
                {
                    parentIndex = entitiesNode[index]["parent"].as<int64_t>();
                }
            }
            catch (const YAML::Exception& exception)
            {
                LOG_WARNING("[SceneSerializer]: Invalid parent index in scene file.");
                LOG_WARNING(exception.what());
            }

            if (parentIndex == NoParentIndex)
            {
                continue;
            }

            if (parentIndex < 0 || static_cast<size_t>(parentIndex) >= entities.size() ||
                static_cast<size_t>(parentIndex) == index)
            {
                LOG_WARNING("[SceneSerializer]: Parent index out of range. Entity remains under scene root.");
                LOG_WARNING(std::to_string(parentIndex));
                continue;
            }

            entities[index].SetParent(entities[static_cast<size_t>(parentIndex)].entity_handle);
        }

        RestoreRuntimeSceneComponents(
            gns::core::SystemsManager::GetRegistry(),
            scene,
            scene.root.entity_handle,
            true);
    }

    void RestoreComponents(
        const YAML::Node& entitiesNode,
        const std::vector<gns::Entity>& entities,
        gns::Scene& scene)
    {
        auto& registry = gns::core::SystemsManager::GetRegistry();
        for (size_t index = 0; index < entities.size(); ++index)
        {
            const YAML::Node componentsNode = entitiesNode[index]["components"];
            if (!componentsNode || !componentsNode.IsMap())
            {
                RestoreRuntimeSceneComponents(registry, scene, entities[index].entity_handle, index == 0);
                continue;
            }

            for (const auto& componentNode : componentsNode)
            {
                try
                {
                    RestoreComponentFields(
                        registry,
                        scene,
                        entities[index].entity_handle,
                        componentNode.first.as<std::string>(),
                        componentNode.second,
                        index == 0);
                }
                catch (const YAML::Exception& exception)
                {
                    LOG_WARNING("[SceneSerializer]: Invalid component entry in scene file.");
                    LOG_WARNING(exception.what());
                }
            }

            RestoreRuntimeSceneComponents(registry, scene, entities[index].entity_handle, index == 0);
        }
    }
}

std::filesystem::path gns::SceneSerializer::GetSceneSavePath(const Scene& scene)
{
    return path::Resolve(path::Root::ProjectAssets, scene.name + ".gnsscene");
}

gns::Scene* gns::SceneSerializer::LoadSceneByName(const std::string& sceneName)
{
    std::filesystem::path scenePath = sceneName;
    if (scenePath.extension() != ".gnsscene")
    {
        scenePath += ".gnsscene";
    }

    return LoadScene(path::Resolve(path::Root::ProjectAssets, scenePath));
}

gns::Scene* gns::SceneSerializer::LoadScene(const std::filesystem::path& scenePath)
{
    const std::filesystem::path loadPath = ResolveSceneLoadPath(scenePath);
    if (!path::IsRegularFile(loadPath))
    {
        LOG_ERROR("[SceneSerializer]: Scene file does not exist.");
        LOG_ERROR(loadPath.string());
        return nullptr;
    }

    YAML::Node document;
    try
    {
        document = YAML::LoadFile(loadPath.string());
    }
    catch (const YAML::Exception& exception)
    {
        LOG_ERROR("[SceneSerializer]: Failed to parse scene file.");
        LOG_ERROR(loadPath.string());
        LOG_ERROR(exception.what());
        return nullptr;
    }

    if (!document["scene"] || !document["serializerVersion"] || !document["entities"])
    {
        LOG_ERROR("[SceneSerializer]: Scene file is missing required fields.");
        LOG_ERROR(loadPath.string());
        return nullptr;
    }

    uint32_t serializerVersion = 0;
    std::string sceneName;
    try
    {
        serializerVersion = document["serializerVersion"].as<uint32_t>();
        sceneName = document["scene"].as<std::string>();
    }
    catch (const YAML::Exception& exception)
    {
        LOG_ERROR("[SceneSerializer]: Scene file has invalid header fields.");
        LOG_ERROR(loadPath.string());
        LOG_ERROR(exception.what());
        return nullptr;
    }

    if (serializerVersion != SerializerVersion)
    {
        LOG_ERROR("[SceneSerializer]: Unsupported scene serializer version.");
        LOG_ERROR(std::to_string(serializerVersion));
        return nullptr;
    }

    const YAML::Node entitiesNode = document["entities"];
    if (!entitiesNode.IsSequence())
    {
        LOG_ERROR("[SceneSerializer]: Scene entities field must be a sequence.");
        LOG_ERROR(loadPath.string());
        return nullptr;
    }

    Scene& scene = SceneManager::CreateScene(sceneName, false);
    SceneManager::SetActiveScene(scene.handle);

    std::vector<Entity> loadedEntities;
    loadedEntities.reserve(entitiesNode.size() > 0 ? entitiesNode.size() : 1);
    loadedEntities.push_back(scene.root);

    auto& registry = core::SystemsManager::GetRegistry();
    if (auto* entityComponent = registry.try_get<EntityComponent>(scene.root.entity_handle))
    {
        entityComponent->name = entitiesNode.size() > 0
            ? ReadEntityName(entitiesNode[0], scene.name)
            : scene.name;
        entityComponent->entity_handle = scene.root.entity_handle;
    }

    for (size_t index = 1; index < entitiesNode.size(); ++index)
    {
        const std::string entityName = ReadEntityName(
            entitiesNode[index],
            "Entity " + std::to_string(index));
        loadedEntities.push_back(Entity::CreateEntity(entityName, scene.handle, scene.root.entity_handle));
    }

    if (entitiesNode.size() > 0)
    {
        RestoreParents(entitiesNode, loadedEntities, scene);
        RestoreComponents(entitiesNode, loadedEntities, scene);
    }
    else
    {
        RestoreRuntimeSceneComponents(registry, scene, scene.root.entity_handle, true);
    }

    LOG_INFO("[SceneSerializer]: Loaded scene.");
    LOG_INFO(loadPath.string());
    return &scene;
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
