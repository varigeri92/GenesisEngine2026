#pragma once
#include "IJob.h"

struct TestJob : public gns::jobs::IJob
{
    GNS_API TestJob(size_t count) : loop_count(count) {};
    
    size_t loop_count = 1000;
    size_t loops_done = 0;
 
    GNS_API void Execute() override;
};
