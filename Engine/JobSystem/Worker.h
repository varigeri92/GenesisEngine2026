#pragma once
#include <thread>

#include "IJob.h"

namespace gns::jobs
{
    struct JobRecord;
}

namespace gns::threading
{
    class Worker
    {
        
        std::thread m_thread;
        std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
        std::atomic<bool> m_isBusy = false;
        
        
    public:
        Worker() = default;
        ~Worker();
        Worker(const Worker&) = delete;
        Worker& operator=(const Worker&) = delete;
        Worker(Worker&&) noexcept = delete;
        Worker& operator=(Worker&&) noexcept = delete;
       
        bool IsFree() const { return !m_isBusy.load(std::memory_order_acquire); }
        void Join();
        bool ScheduleJob(gns::jobs::JobRecord* job, jobs::JobHandle handle);

        void Lock();
        void Unlock();
    };
}
