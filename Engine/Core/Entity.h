#pragma once
#include <string>
#include <utility>
#include <vector>

#include "entt/entt.hpp"
#include "Handles.h"
#include "../Systems/SystemsManager.h"
namespace gns
{
    typedef entt::entity entityHandle;
    
    struct Entity
    {
        entityHandle entity_handle;
        Entity(entityHandle entity_handle) :
            entity_handle{ entity_handle }
        {}
        Entity() : entity_handle{ entt::null } {};

        Entity& operator=(const Entity& other)
        {
            entity_handle = other.entity_handle;
            return *this;
        }

        bool operator==(const Entity& other) const {
            return entity_handle == other.entity_handle;
        }

        operator entityHandle() const
        {
            return entity_handle;
        }
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
        T& GetComponent()
        {
            return gns::core::SystemsManager::GetRegistry().get<T>(entity_handle);
        }

        template<typename T>
        bool TryGetComponent(T*& component)
        {
            component = core::SystemsManager::GetRegistry().try_get<T>(entity_handle);
            return component != nullptr;
        }

        GNS_API static Entity CreateEntity(const std::string& entityName);
        GNS_API static Entity CreateEntity(
            const std::string& entityName,
            Handle sceneHandle,
            entityHandle parent = entt::null);

        GNS_API Entity Parent() const;
        GNS_API const std::vector<gns::entityHandle>& Children() const;
        GNS_API void SetParent(entityHandle parent);
        
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
