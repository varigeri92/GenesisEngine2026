#include "gnspch.h"
#include "TestJob.h"

void TestJob::Execute()
{
    for (size_t i = 0; i < loop_count; ++i)
    {
        loops_done++;
    }
}
