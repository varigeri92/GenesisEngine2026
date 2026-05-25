#pragma once
#include <thread>

#include "../Core/Handles.h"
#include "../Systems/system.h"
#include "../Utils/Path.h"

namespace gns::jobs
{
    struct IJob;
    class JobSystem;
}
template<typename jobType>
concept DerivedFromJob = std::derived_from<jobType, gns::jobs::IJob>;
namespace gns::jobs
{
    struct JobHandle
    {
        bool operator==(const JobHandle& other) const
        {
            return handle == other.handle;
        }
        size_t handle = 0;
    };
    struct IJob
    {
        virtual ~IJob() = default;
        GNS_API virtual void Execute() = 0;
    };
}

namespace std
{
    template<>
    struct hash<gns::jobs::JobHandle>
    {
        size_t operator()(const gns::jobs::JobHandle& jobHandle) const noexcept
        {
            return jobHandle.handle;
        }
    };
}
