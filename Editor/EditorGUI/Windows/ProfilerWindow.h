#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "GenesisGUI.h"
#include "../../../Engine/Profiling/Profiler.h"

class ProfilerWindow : public GuiWindow
{
public:
	explicit ProfilerWindow(std::string title);

	void OnDraw() override;

private:
	void RefreshSnapshot();
	void RefreshFrameOverview();
	void SelectFrameByIndex(uint64_t frameIndex);
	void ExportTrace();
	void DrawToolbar();
	void DrawFrameGraph();
	void DrawSelectedFrame();
	void DrawTimeline(const gns::profiling::ProfileFrameSample& frame);
	void DrawScopeTable(const gns::profiling::ProfileFrameSample& frame);

	std::vector<gns::profiling::ProfileFrameSample> m_frames;
	std::vector<gns::profiling::ProfileFrameOverview> m_frameOverview;
	std::vector<float> m_frameTimesMs;
	bool m_freezeView = false;
	bool m_followLatest = true;
	bool m_showScopeTable = false;
	int m_selectedFrameOffsetFromLatest = 0;
	int m_frameHistoryLimit = 300;
	float m_timelineZoom = 1.0f;
	int64_t m_timelinePanUs = 0;
	std::string m_exportStatus;
};
