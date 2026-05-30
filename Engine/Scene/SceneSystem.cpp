#include "gnspch.h"
#include "SceneSystem.h"

#include "SceneAssetImporter.h"
#include "SceneManager.h"

void gns::SceneSystem::OnCreate()
{
}

void gns::SceneSystem::OnStart()
{
    if (SceneManager::TryGetActiveScene() == nullptr)
    {
        SceneManager::CreateScene("empty(scene)");
    }
}

void gns::SceneSystem::OnUpdate(float deltaTime)
{
    SceneAssetImporter::UpdatePendingImports();
    //traverse hierarchy update transforms.
}

void gns::SceneSystem::OnDestroy()
{
    SceneManager::Clear();
}
