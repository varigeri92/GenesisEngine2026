#pragma once
#include "Genesis.h"

class TestSystemExternal : public gns::core::System
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

	TestSystemExternal() = default;
	void OnLateUpdate(float deltaTime) override;
};

