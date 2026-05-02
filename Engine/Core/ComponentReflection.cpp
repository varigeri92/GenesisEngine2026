#include "gnspch.h"
#include "ComponentReflection.h"

namespace
{
    std::vector<gns::reflection::ComponentMeta>& ComponentMetas()
    {
        static std::vector<gns::reflection::ComponentMeta> components;
        return components;
    }

    std::unordered_map<gns::Handle, size_t>& ComponentMetaIndex()
    {
        static std::unordered_map<gns::Handle, size_t> index;
        return index;
    }
}

const std::vector<gns::reflection::ComponentMeta>& gns::reflection::ComponentRegistry::GetComponents()
{
    return ComponentMetas();
}

gns::reflection::ComponentMeta* gns::reflection::ComponentRegistry::GetComponentMeta(Handle componentTypeId)
{
    auto& index = ComponentMetaIndex();
    const auto it = index.find(componentTypeId);
    if (it == index.end())
    {
        return nullptr;
    }

    return &ComponentMetas()[it->second];
}

gns::reflection::ComponentMeta* gns::reflection::ComponentRegistry::GetComponentMeta(
    const std::string& componentName)
{
    return GetComponentMeta(Handle::CreateFromString(componentName));
}

gns::reflection::ComponentMeta& gns::reflection::ComponentRegistry::RegisterComponentInternal(
    Handle typeId,
    const std::string& componentName,
    size_t componentSize,
    ComponentHasFn hasComponent,
    ComponentGetFn getComponent)
{
    auto& components = ComponentMetas();
    auto& index = ComponentMetaIndex();

    if (const auto it = index.find(typeId); it != index.end())
    {
        ComponentMeta& existing = components[it->second];
        existing.has_component = hasComponent;
        existing.get_component = getComponent;
        return existing;
    }

    ComponentMeta meta;
    meta.type_id = typeId;
    meta.name = componentName;
    meta.size = componentSize;
    meta.has_component = hasComponent;
    meta.get_component = getComponent;

    const size_t metaIndex = components.size();
    components.emplace_back(std::move(meta));
    index[typeId] = metaIndex;
    return components[metaIndex];
}

void gns::reflection::ComponentRegistry::RegisterFieldInternal(
    Handle componentTypeId,
    FieldMeta fieldMeta)
{
    ComponentMeta* componentMeta = GetComponentMeta(componentTypeId);
    if (componentMeta == nullptr)
    {
        return;
    }

    const auto existingField = std::find_if(
        componentMeta->fields.begin(),
        componentMeta->fields.end(),
        [&](const FieldMeta& field)
        {
            return field.name == fieldMeta.name;
        });

    if (existingField != componentMeta->fields.end())
    {
        return;
    }

    componentMeta->fields.emplace_back(std::move(fieldMeta));
}
