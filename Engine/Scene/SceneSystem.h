#pragma once
#include "../Systems/System.h"

namespace gns
{
    class SceneSystem : public gns::core::System
    {
    public:
        ~SceneSystem() override = default;
        void OnCreate() override;
        void OnStart() override;
        void OnUpdate(float deltaTime) override;
        void OnDestroy() override;
        
        
    };
    
}
