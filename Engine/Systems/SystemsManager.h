#pragma once
#include <vector>
#include "system.h"
#include <memory>
#include <entt/entt.hpp>

namespace gns::core
{

	class SystemsManager
	{
	public:
		template<typename System_T>
		static System_T* RegisterSystem()
		{
			Systems.emplace_back(std::make_unique<System_T>());
			Systems[Systems.size() - 1]->State = System::SystemState::Created;
			return reinterpret_cast<System_T*>(Systems[Systems.size() - 1].get());
		}
		template
			<typename System_T, 
			typename = std::enable_if<std::is_base_of<System, System_T>::value>::type, 
			typename... Args>
		static System_T* RegisterSystem(Args&& ... args)
		{
			Systems.emplace_back(std::make_unique<System_T>(std::forward<Args>(args)...));
			Systems[Systems.size() - 1]->State = System::SystemState::Created;
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
		
		GNS_API static entt::registry& GetRegistry();
	private:
		GNS_API static std::vector<std::unique_ptr<System>> Systems;
		GNS_API static std::vector<size_t> deletionQeue;
		GNS_API static entt::registry Registry;
	};

}