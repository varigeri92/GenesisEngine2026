#include "gnspch.h"
#include "JobSystem.h"

#include <thread>

size_t gns::jobs::JobSystem::MAX_THREADS = std::thread::hardware_concurrency();
size_t gns::jobs::JobSystem::m_nextHandle = 1;
std::unordered_map<gns::jobs::JobHandle, gns::jobs::JobRecord> gns::jobs::JobSystem::Jobs = {};
std::vector<gns::jobs::JobHandle> gns::jobs::JobSystem::Released = {};
std::vector<std::unique_ptr<gns::threading::Worker>> gns::jobs::JobSystem::Workers = {};

void gns::jobs::JobSystem::Schedule(JobRecord& job, JobHandle handle)
{
    bool has_free_worker = false;
    for (int i = 0; i < Workers.size(); ++i)
    {
        if (Workers[i]->IsFree())
        {
            job.state.store(JobState::Running, std::memory_order_release);
            has_free_worker = true;
            if (!Workers[i]->ScheduleJob(&job, handle))
            {
                job.state.store(JobState::Pending, std::memory_order_release);
            }
            break;
        }
    }
    if (has_free_worker == false)
    {
        if (Workers.size() >= MAX_THREADS - 2)
        {
            return;
        }
        job.state.store(JobState::Running, std::memory_order_release);
        Workers.emplace_back(std::make_unique<gns::threading::Worker>());
        if(!Workers.back()->ScheduleJob(&job, handle))
        {
            job.state.store(JobState::Pending, std::memory_order_release);
        }
    }
}

void gns::jobs::JobSystem::FlushJobs()
{
    for (auto pair = Jobs.begin(); pair != Jobs.end(); ++pair)
    {
        if (pair->second.state.load(std::memory_order_acquire) == JobState::Pending)
        {
            Schedule(pair->second, pair->first);
        }
    }
    for (size_t i = 0; i < Released.size(); ++i)
    {
        Jobs.erase(Released[i]);
    }
    Released.clear();
}

void gns::jobs::JobSystem::Initialize()
{
    MAX_THREADS = std::thread::hardware_concurrency();
    Released.reserve(MAX_THREADS);
    Jobs.reserve(MAX_THREADS);
    Workers.reserve(MAX_THREADS);
}

void gns::jobs::JobSystem::MarkCompleted(JobHandle handle)
{
    auto job = Jobs.find(handle);
    if (job != Jobs.end())
        job->second.state.store(JobState::Completed, std::memory_order_release);
}

void gns::jobs::JobSystem::Schedule(JobHandle handle)
{
    auto job = Jobs.find(handle);
    if (job == Jobs.end())
    {
        LOG_ERROR("Job not created! Use JobSystem::CreateJob<Job_Type>(Args&& ... args)");
        return;
    }
    if (job->second.state.load(std::memory_order_acquire) != JobState::Pending)
    {
        LOG_WARNING("Job already scheduled. running or completed!");
        return;
    }
    Schedule(job->second, handle);
}

void gns::jobs::JobSystem::Release(JobHandle handle)
{
    auto job = Jobs.find(handle);
    if (job != Jobs.end())
    {
        job->second.state.store(JobState::Released, std::memory_order_release);
        Released.emplace_back(handle);
    }
}
