#pragma once
#include "IJob.h"
#include "Worker.h"
#include "../Core/EntityHandle.h"

namespace gns::jobs
{
    enum class JobState
    {
        Pending,
        Running,
        Completed,
        Failed,
        Released
    };

    struct JobRecord
    {
        explicit JobRecord(std::unique_ptr<IJob> job)
            : job(std::move(job))
        {
        }

        std::unique_ptr<IJob> job;
        std::atomic<JobState> state = JobState::Pending;
    };
    
    class JobSystem
    {
        friend struct IJob;
    public:
        GNS_API static size_t MAX_THREADS;
        GNS_API static std::unordered_map<JobHandle, JobRecord> Jobs;
        GNS_API static std::vector<JobHandle> Released;
        GNS_API static std::vector<std::unique_ptr<gns::threading::Worker>> Workers;
        GNS_API static void Schedule(JobRecord& job, JobHandle handle);
        GNS_API static void FlushJobs();
        GNS_API static void Initialize();
        GNS_API static void MarkCompleted(JobHandle handle);
        GNS_API static size_t m_nextHandle;
        template <DerivedFromJob JobType>
        static const JobHandle CreateJob(JobType job)
        {
            JobHandle handle = JobHandle{ .handle = m_nextHandle };
            m_nextHandle++;

            Jobs.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(handle),
                std::forward_as_tuple(std::make_unique<JobType>(std::move(job))));
            return handle;
        }
        GNS_API static void Schedule(JobHandle handle);
        template <DerivedFromJob JobType>
        static JobType* GetCompleted(JobHandle handle)
        {
            auto job = Jobs.find(handle);
            if (job != Jobs.end())
            {
                if (job->second.state.load(std::memory_order_acquire) == jobs::JobState::Completed)
                {
                    return static_cast<JobType*>(job->second.job.get());
                }
            }
            return nullptr;
        }
        GNS_API static void Release(JobHandle handle);
    };
}
