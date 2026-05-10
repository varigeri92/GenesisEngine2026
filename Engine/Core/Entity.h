#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "EntityHandle.h"
#include "Handles.h"
#include "../Systems/SystemsManager.h"

namespace gns
{
    struct Entity
    {
        entityHandle entity_handle;
        Entity(entityHandle entity_handle) :
            entity_handle{ entity_handle }
        {}
        Entity() : entity_handle{ NullEntity } {};

        Entity& operator=(Entity other)
        {
            entity_handle = other.entity_handle;
            return *this;
        }

        bool operator==(Entity other) const {
            return entity_handle == other.entity_handle;
        }

        bool operator==(entityHandle other) const
        {
            return entity_handle == other;
        }

        bool operator!=(Entity other) const
        {
            return !(*this == other);
        }

        bool operator!=(entityHandle other) const
        {
            return entity_handle != other;
        }

        operator entityHandle() const
        {
            return entity_handle;
        }

        GNS_API entityHandle GetHandle() const;
        GNS_API uint32_t GetDebugId() const;
        GNS_API bool IsValid() const;
        operator bool() const { return IsValid(); }

        GNS_API void Delete();

        template<typename T, typename... Args>
        T& AddComponent(Args&& ... args)
        {
            T& component = gns::core::SystemsManager::GetRegistry()
                .emplace<T>(entity_handle, std::forward<Args>(args)...);
            return component;
        }
        
        template<typename T>
        T& AddComponent()
        {
            T& component = gns::core::SystemsManager::GetRegistry()
                .emplace<T>(entity_handle);
            return component;
        }
        
        template<typename T>
        bool HasComponent() const
        {
            return IsValid() && core::SystemsManager::GetRegistry().any_of<T>(entity_handle);
        }

        template<typename T>
        T& GetComponent()
        {
            return gns::core::SystemsManager::GetRegistry().get<T>(entity_handle);
        }

        template<typename T>
        const T& GetComponent() const
        {
            const auto& registry = gns::core::SystemsManager::GetRegistry();
            return registry.get<T>(entity_handle);
        }

        template<typename T, typename... Args>
        T& EnsureComponent(Args&& ... args)
        {
            return gns::core::SystemsManager::GetRegistry()
                .get_or_emplace<T>(entity_handle, std::forward<Args>(args)...);
        }

        template<typename T>
        T* TryGetComponent()
        {
            return core::SystemsManager::GetRegistry().try_get<T>(entity_handle);
        }
        
        template<typename T>
        const T* TryGetComponent() const
        {
            const auto& registry = core::SystemsManager::GetRegistry();
            return registry.try_get<T>(entity_handle);
        }

        template<typename T>
        bool TryGetComponent(T*& component)
        {
            component = TryGetComponent<T>();
            return component != nullptr;
        }

        template<typename T>
        bool TryGetComponent(const T*& component) const
        {
            component = TryGetComponent<T>();
            return component != nullptr;
        }

        template<typename T>
        bool RemoveComponent()
        {
            if (!HasComponent<T>())
            {
                return false;
            }

            core::SystemsManager::GetRegistry().remove<T>(entity_handle);
            return true;
        }

        GNS_API static entityHandle InvalidHandle();
        GNS_API static bool IsValidHandle(entityHandle entity);
        GNS_API static Entity CreateEntity(const std::string& entityName);
        GNS_API static Entity CreateEntity(
            const std::string& entityName,
            Handle sceneHandle,
            entityHandle parent = NullEntity);
        GNS_API static Entity CreateSceneRoot(const std::string& entityName, Handle sceneHandle);
        GNS_API static void DestroyTree(entityHandle entity, bool allowSceneRoot = false);

        GNS_API const std::string& Name() const;
        GNS_API void SetName(const std::string& newName);
        GNS_API Handle SceneHandle() const;
        GNS_API bool IsSceneRoot() const;
        GNS_API Entity Parent() const;
        GNS_API const std::vector<gns::entityHandle>& Children() const;
        GNS_API void SetParent(entityHandle parent);

        void SetParent(Entity parent)
        {
            SetParent(parent.entity_handle);
        }
        
        /* 
        GNS_API const std::vector<gns::ComponentData>& GetAllComponent();
        GNS_API entity::Transform& Transform();
        GNS_API void AddChild(entityHandle entity_handle);
        GNS_API void RemoveChild(entityHandle entity);
        GNS_API const std::string& Name();
        GNS_API void SetName(const std::string& newName);
        GNS_API guid GetGuid();
        static Entity CreateEntity_Internal(const std::string& entityName, const guid guid, gns::scene::Scene* scene);
    private:
        std::vector<gns::ComponentData> componentsVector;
        */
    };
}
