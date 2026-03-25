#define SDL_MAIN_HANDLED
#include <iostream>
#include "Genesis.h"
#include "TestSystemExternal.h"
#include "../Engine/Renderer/RenderSystem.h"
#include "../Window/WindowSystem.h"

int main()
{
    gns::core::EngineConfig cfg = {};
    cfg.headless = false;
    cfg.InitTetsSystem = true;


    gns::core::Engine engine(cfg);
    engine.Initialize([&]() {
        gns::core::SystemsManager::RegisterSystem<TestSystemExternal>();
    });
    engine.Run();
    engine.ShutDown();
    std::getchar();
}

