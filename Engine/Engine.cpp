#include "gnspch.h"
#include "Engine.h"

gns::core::Engine::Engine()
{
	std::cout << "Hello Engine!" << std::endl;
}

void gns::core::Engine::Initialize()
{
	std::cout << "Hello Initialize!" << std::endl;
}

void gns::core::Engine::Run()
{
	std::cout << "Engine Running!" << std::endl;
}

void gns::core::Engine::ShutDown()
{
	std::cout << "Engine Shut Down!" << std::endl;
}
