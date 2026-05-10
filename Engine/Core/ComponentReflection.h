#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "Entity.h"
#include "Handles.h"

namespace gns::reflection
{
    enum class FieldKind : uint8_t
    {
        Bool,
        Int,
        Float,
        String,
        Vec2,
        Vec3,
        Vec4,
        Handle,
        EntityHandle,
        Reference
    };

    enum class FieldFlags : uint32_t
    {
        None = 0,
        Hidden = 1 << 0,
        EditorReadOnly = 1 << 1
    };

    constexpr FieldFlags operator|(FieldFlags left, FieldFlags right)
    {
        return static_cast<FieldFlags>(
            static_cast<uint32_t>(left) | static_cast<uint32_t>(right));
    }

    constexpr bool HasFlag(FieldFlags flags, FieldFlags flag)
    {
        return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
    }

    using ComponentHasFn = bool (*)(Entity entity);
    using ComponentGetFn = void* (*)(Entity entity);
    using ComponentEnsureFn = void* (*)(Entity entity);

    struct FieldMeta
    {
        std::string name;
        std::string type_name;
        std::string reference_type_name;
        FieldKind kind = FieldKind::Float;
        FieldFlags flags = FieldFlags::None;
        size_t size = 0;
        size_t alignment = 0;
        std::function<void*(void*)> get_field;
        std::function<const void*(const void*)> get_const_field;
        std::function<void(void*, const void*)> set_field;
        std::function<uint64_t(void*)> get_reference_handle;
        std::function<void(void*, uint64_t)> set_reference_handle;
        std::function<size_t(void*)> get_reference_type_id;
        std::function<void(void*, size_t)> set_reference_type_id;

        bool IsHidden() const { return HasFlag(flags, FieldFlags::Hidden); }
        bool IsEditorReadOnly() const { return HasFlag(flags, FieldFlags::EditorReadOnly); }
    };

    struct ComponentMeta
    {
        Handle type_id;
        std::string name;
        size_t size = 0;
        std::vector<FieldMeta> fields;
        ComponentHasFn has_component = nullptr;
        ComponentGetFn get_component = nullptr;
        ComponentEnsureFn ensure_component = nullptr;
    };

    template<typename Component>
    void RegisterFields(ComponentMeta& componentMeta)
    {
        (void)componentMeta;
    }

    template<typename T>
    std::string TypeName()
    {
        std::string name = typeid(T).name();
        const std::string structPrefix = "struct ";
        const std::string classPrefix = "class ";

        size_t position = 0;
        while ((position = name.find(structPrefix, position)) != std::string::npos)
        {
            name.erase(position, structPrefix.size());
        }

        position = 0;
        while ((position = name.find(classPrefix, position)) != std::string::npos)
        {
            name.erase(position, classPrefix.size());
        }

        return name;
    }

    inline std::string ReferenceTargetTypeName(const std::string& referenceTypeName)
    {
        const size_t start = referenceTypeName.find('<');
        const size_t end = referenceTypeName.rfind('>');
        if (start == std::string::npos || end == std::string::npos || end <= start + 1)
        {
            return referenceTypeName;
        }

        return referenceTypeName.substr(start + 1, end - start - 1);
    }

    template<typename T>
    struct FieldKindResolver
    {
        static constexpr bool Supported = false;
        static constexpr FieldKind Kind = FieldKind::Float;
    };

    template<> struct FieldKindResolver<bool> { static constexpr bool Supported = true; static constexpr FieldKind Kind = FieldKind::Bool; };
    template<> struct FieldKindResolver<int> { static constexpr bool Supported = true; static constexpr FieldKind Kind = FieldKind::Int; };
    template<> struct FieldKindResolver<float> { static constexpr bool Supported = true; static constexpr FieldKind Kind = FieldKind::Float; };
    template<> struct FieldKindResolver<std::string> { static constexpr bool Supported = true; static constexpr FieldKind Kind = FieldKind::String; };
    template<> struct FieldKindResolver<glm::vec2> { static constexpr bool Supported = true; static constexpr FieldKind Kind = FieldKind::Vec2; };
    template<> struct FieldKindResolver<glm::vec3> { static constexpr bool Supported = true; static constexpr FieldKind Kind = FieldKind::Vec3; };
    template<> struct FieldKindResolver<glm::vec4> { static constexpr bool Supported = true; static constexpr FieldKind Kind = FieldKind::Vec4; };
    template<> struct FieldKindResolver<gns::Handle> { static constexpr bool Supported = true; static constexpr FieldKind Kind = FieldKind::Handle; };
    template<> struct FieldKindResolver<gns::entityHandle> { static constexpr bool Supported = true; static constexpr FieldKind Kind = FieldKind::EntityHandle; };

    template<typename T>
    struct IsReference : std::false_type {};

    template<typename T>
    struct IsReference<gns::Reference<T>> : std::true_type
    {
        using ReferenceType = T;
    };

    template<typename T>
    struct FieldKindResolver<gns::Reference<T>>
    {
        static constexpr bool Supported = true;
        static constexpr FieldKind Kind = FieldKind::Reference;
    };

    class ComponentRegistry
    {
    public:
        template<typename Component>
        static ComponentMeta& RegisterComponent()
        {
            const std::string componentName = TypeName<Component>();
            ComponentMeta& componentMeta = RegisterComponentInternal(
                Handle::CreateFromString(componentName),
                componentName,
                sizeof(Component),
                &HasComponent<Component>,
                &GetComponent<Component>,
                &EnsureComponent<Component>);

            RegisterFields<Component>(componentMeta);
            return componentMeta;
        }

        template<typename Component, typename Field>
        static void RegisterField(
            ComponentMeta& componentMeta,
            const char* fieldName,
            Field Component::* member,
            FieldFlags flags = FieldFlags::None)
        {
            using FieldType = std::remove_cv_t<Field>;
            static_assert(
                FieldKindResolver<FieldType>::Supported,
                "RegisterField used with an unsupported field type.");

            FieldMeta fieldMeta;
            fieldMeta.name = fieldName;
            fieldMeta.type_name = TypeName<FieldType>();
            fieldMeta.kind = FieldKindResolver<FieldType>::Kind;
            fieldMeta.flags = flags;
            fieldMeta.size = sizeof(FieldType);
            fieldMeta.alignment = alignof(FieldType);
            fieldMeta.get_field = [member](void* component) -> void*
            {
                return &(static_cast<Component*>(component)->*member);
            };
            fieldMeta.get_const_field = [member](const void* component) -> const void*
            {
                return &(static_cast<const Component*>(component)->*member);
            };
            fieldMeta.set_field = [member](void* component, const void* value)
            {
                static_cast<Component*>(component)->*member = *static_cast<const FieldType*>(value);
            };

            if constexpr (IsReference<FieldType>::value)
            {
                fieldMeta.reference_type_name = ReferenceTargetTypeName(fieldMeta.type_name);
                fieldMeta.get_reference_handle = [](void* field) -> uint64_t
                {
                    return static_cast<FieldType*>(field)->m_handle.Get();
                };
                fieldMeta.set_reference_handle = [](void* field, uint64_t handle)
                {
                    static_cast<FieldType*>(field)->m_handle = Handle::Create(handle);
                };
                fieldMeta.get_reference_type_id = [](void* field) -> size_t
                {
                    return static_cast<FieldType*>(field)->typeID;
                };
                fieldMeta.set_reference_type_id = [](void* field, size_t typeId)
                {
                    static_cast<FieldType*>(field)->typeID = typeId;
                };
            }

            RegisterFieldInternal(componentMeta.type_id, std::move(fieldMeta));
        }

        GNS_API static const std::vector<ComponentMeta>& GetComponents();
        GNS_API static ComponentMeta* GetComponentMeta(Handle componentTypeId);
        GNS_API static ComponentMeta* GetComponentMeta(const std::string& componentName);

    private:
        template<typename Component>
        static bool HasComponent(Entity entity)
        {
            return entity.HasComponent<Component>();
        }

        template<typename Component>
        static void* GetComponent(Entity entity)
        {
            return entity.TryGetComponent<Component>();
        }

        template<typename Component>
        static void* EnsureComponent(Entity entity)
        {
            if (!entity.IsValid())
            {
                return nullptr;
            }

            return &entity.EnsureComponent<Component>();
        }

        GNS_API static ComponentMeta& RegisterComponentInternal(
            Handle typeId,
            const std::string& componentName,
            size_t componentSize,
            ComponentHasFn hasComponent,
            ComponentGetFn getComponent,
            ComponentEnsureFn ensureComponent);

        GNS_API static void RegisterFieldInternal(Handle componentTypeId, FieldMeta fieldMeta);
    };

}

#define GNS_HIDDEN ::gns::reflection::FieldFlags::Hidden
#define GNS_EDITOR_READONLY ::gns::reflection::FieldFlags::EditorReadOnly
