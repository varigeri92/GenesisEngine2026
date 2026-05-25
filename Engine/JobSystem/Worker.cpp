#include "gnspch.h"
#include "Worker.h"

#include "JobSystem.h"

void gns::threading::Worker::Lock()
{
    while (m_flag.test_and_set(std::memory_order_acquire))
    {
        _YIELD_PROCESSOR();
    }
}

void gns::threading::Worker::Unlock()
{
    m_flag.clear(std::memory_order_release);
}

bool gns::threading::Worker::ScheduleJob(gns::jobs::JobRecord* job, jobs::JobHandle handle)
{
    bool expected = false;
    if (!m_isBusy.compare_exchange_strong(expected, true))
    {
        return false;
    }
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    m_thread = std::thread([this, job]()
    {
        job->job->Execute();
        job->state.store(jobs::JobState::Completed, std::memory_order_release);
        m_isBusy.store(false, std::memory_order_release);
        LOG_INFO("Job Completed!");
    });
    return true;
}

void gns::threading::Worker::Join()
{
    if (m_thread.joinable())
        m_thread.join();    
}

gns::threading::Worker::~Worker()
{
    if (m_thread.joinable())
        m_thread.join();
}
