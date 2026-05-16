#pragma once
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "system.h"
#include <entt/entt.hpp>
#include "../Core/EntityHandle.h"

namespace gns::core
{

	class SystemsManager
	{
	public:
		template<typename System_T>
		static System_T* RegisterSystem()
		{
			Systems.emplace_back(std::make_unique<System_T>());
			System_T* System = reinterpret_cast<System_T*>(Systems[Systems.size() - 1].get());
			System->State = System::SystemState::Created;
			System->metadata.name = typeid(System_T).name();
			return System;
		}
		template
			<typename System_T, 
			typename = std::enable_if<std::is_base_of<System, System_T>::value>::type, 
			typename... Args>
		static System_T* RegisterSystem(Args&& ... args)
		{
			Systems.emplace_back(std::make_unique<System_T>(std::forward<Args>(args)...));
			Systems[Systems.size() - 1]->State = System::SystemState::Created;
			Systems[Systems.size() - 1]->metadata.name = typeid(System_T).name();
			return dynamic_cast<System_T*>(Systems[Systems.size() - 1].get());
		}
		template
			<typename System_T, 
			typename = std::enable_if<std::is_base_of<System, System_T>::value>::type, 
			typename... Args>
		static System_T* GetSystem()
		{
			for (const auto& system : Systems)
			{
				if (typeid(*system.get()) == typeid(System_T))
					return static_cast<System_T*>(system.get());
			}
			return nullptr;
		}
		static void Run(float deltaTime);
		static void Clear();

		GNS_API static bool IsEntityValid(gns::entityHandle entity);

		template<typename... Component_T, typename Func>
		static void ForEach(Func&& func)
		{
			auto view = Registry.view<Component_T...>();
			for (gns::entityHandle entity : view)
			{
				if constexpr (std::is_invocable_v<Func&, gns::entityHandle, Component_T&...>)
				{
					std::invoke(func, entity, view.template get<Component_T>(entity)...);
				}
				else
				{
					static_assert(
						std::is_invocable_v<Func&, Component_T&...>,
						"SystemsManager::ForEach callback must accept either (entityHandle, components...) or (components...).");
					std::invoke(func, view.template get<Component_T>(entity)...);
				}
			}
		}
		
		GNS_API static entt::registry& GetRegistry();
		GNS_API static std::vector<std::unique_ptr<System>> Systems;
	private:
		GNS_API static std::vector<size_t> deletionQeue;
		GNS_API static entt::registry Registry;
	};

}
