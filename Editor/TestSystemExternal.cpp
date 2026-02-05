#include "TestSystemExternal.h"

void TestSystemExternal::OnCreate()
{
}

void TestSystemExternal::OnStart()
{
}

void TestSystemExternal::OnEnable()
{
}

void TestSystemExternal::OnUpdate(float deltaTime)
{
	if (gns::core::InputBackend::GetKeyDown(119))
	{
		LOG_INFO("W down");
	}
}

void TestSystemExternal::OnFixedUpdate()
{
}

void TestSystemExternal::OnDisable()
{
}

void TestSystemExternal::OnDestroy()
{
}
