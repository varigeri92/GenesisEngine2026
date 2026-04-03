#include "gnspch.h"
#include "SceneSystem.h"

#include "SceneManager.h"

void gns::SceneSystem::OnCreate()
{
}

void gns::SceneSystem::OnStart()
{
    SceneManager::CreateScene("empty(scene)");
}

void gns::SceneSystem::OnUpdate(float deltaTime)
{
    //traverse hierarchy update transforms.
}

void gns::SceneSystem::OnDestroy()
{
    //Unload Scenes;
}
