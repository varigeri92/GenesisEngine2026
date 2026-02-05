#pragma once
#include "System.h"
namespace gns::core {

class TestSystem : public System
{

	// Inherited via System
	void OnCreate() override;
	void OnStart() override;
	void OnEnable() override;
	void OnUpdate(float deltaTime) override;
	void OnFixedUpdate() override;
	void OnDisable() override;
	void OnDestroy() override;

public:
	TestSystem() = default;
	~TestSystem();
};
}

