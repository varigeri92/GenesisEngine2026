#include "TestEditorWindow.h"
#include "GenesisGUI.h"
#include "../Engine/Systems/SystemsManager.h"
#include "EditorGUI/EditorWidgets.h"
#include "Genesis.h"
#include "../Engine/JobSystem/JobSystem.h"
#include "../Engine/JobSystem/TestJob.h"

int counter = 0;

std::vector<gns::jobs::JobHandle> jobsCreated = {};

void TestEditorWindow::OnDraw()
{
    std::string max_threads = std::to_string(gns::jobs::JobSystem::MAX_THREADS);
    std::string cur_threads = std::to_string(gns::jobs::JobSystem::Workers.size());
    ImGui::Text((max_threads + "/" + cur_threads ).c_str());
    ImGui::Separator();
    std::string Jobs_running = std::to_string(gns::jobs::JobSystem::Jobs.size());
    ImGui::Text((Jobs_running).c_str());
    ImGui::Separator();
    ImGui::DragInt("Loops:", &counter);
    if (ImGui::Button("Start A job"))
    {
        TestJob	test_job(static_cast<size_t>(100) * static_cast<size_t>(counter));
        const gns::jobs::JobHandle job_handle = gns::jobs::JobSystem::CreateJob<TestJob>(test_job);
        gns::jobs::JobSystem::Schedule(job_handle);
        jobsCreated.push_back(job_handle);
    }

    for (auto job_created : jobsCreated)
    {
        if (TestJob* job = gns::jobs::JobSystem::GetCompleted<TestJob>(job_created))
        {
            LOG_INFO(std::to_string(job->loops_done));
            gns::jobs::JobSystem::Release(job_created);
        }
    }
}
