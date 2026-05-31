#include "ProfilerWindow.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>

#include "GenesisMaterialIcons.h"
#include "../../../Engine/Utils/Path.h"

namespace
{
	constexpr float GraphHeight = 140.0f;
	constexpr float TimelineLabelWidth = 92.0f;
	constexpr float TimelineHeaderHeight = 22.0f;
	constexpr float TimelineLaneHeight = 20.0f;
	constexpr float TimelineBarHeight = 14.0f;
	constexpr float TimelineThreadGap = 6.0f;
	constexpr float TimelineViewportHeight = 260.0f;
	constexpr double MicrosecondsToMilliseconds = 0.001;

	struct TimelineBar
	{
		const gns::profiling::ProfileScopeSample* scope = nullptr;
		std::size_t lane = 0;
	};

	struct TimelineThread
	{
		uint32_t threadId = 0;
		std::vector<TimelineBar> bars;
		std::size_t laneCount = 1;
	};

	float MaxFrameTime(const std::vector<float>& frameTimes)
	{
		if (frameTimes.empty())
		{
			return 33.333f;
		}

		const float maxFrameTime = *std::max_element(frameTimes.begin(), frameTimes.end());
		return std::max(maxFrameTime, 33.333f);
	}

	const gns::profiling::ProfileFrameSample* SelectedFrame(
		const std::vector<gns::profiling::ProfileFrameSample>& frames,
		int selectedFrameOffsetFromLatest)
	{
		if (frames.empty())
		{
			return nullptr;
		}

		const int lastFrameIndex = static_cast<int>(frames.size()) - 1;
		const int clampedOffset = std::clamp(selectedFrameOffsetFromLatest, 0, lastFrameIndex);
		return &frames[static_cast<std::size_t>(lastFrameIndex - clampedOffset)];
	}

	std::vector<gns::profiling::ProfileScopeSample> SortedScopes(
		const gns::profiling::ProfileFrameSample& frame)
	{
		std::vector<gns::profiling::ProfileScopeSample> scopes = frame.scopes;
		std::sort(scopes.begin(), scopes.end(), [](const auto& left, const auto& right)
		{
			return left.durationUs > right.durationUs;
		});

		return scopes;
	}

	std::vector<TimelineThread> BuildTimelineThreads(const gns::profiling::ProfileFrameSample& frame)
	{
		std::vector<uint32_t> threadIds;
		for (const gns::profiling::ProfileScopeSample& scope : frame.scopes)
		{
			if (std::find(threadIds.begin(), threadIds.end(), scope.threadId) == threadIds.end())
			{
				threadIds.push_back(scope.threadId);
			}
		}

		std::sort(threadIds.begin(), threadIds.end());

		std::vector<TimelineThread> threads;
		threads.reserve(threadIds.size());

		for (const uint32_t threadId : threadIds)
		{
			std::vector<const gns::profiling::ProfileScopeSample*> scopes;
			for (const gns::profiling::ProfileScopeSample& scope : frame.scopes)
			{
				if (scope.threadId == threadId)
				{
					scopes.push_back(&scope);
				}
			}

			std::sort(scopes.begin(), scopes.end(), [](const auto* left, const auto* right)
			{
				if (left->startUs != right->startUs)
				{
					return left->startUs < right->startUs;
				}

				return left->durationUs > right->durationUs;
			});

			TimelineThread thread;
			thread.threadId = threadId;
			std::vector<int64_t> laneEndTimes;

			for (const gns::profiling::ProfileScopeSample* scope : scopes)
			{
				std::size_t lane = 0;
				for (; lane < laneEndTimes.size(); ++lane)
				{
					if (scope->startUs >= laneEndTimes[lane])
					{
						break;
					}
				}

				if (lane == laneEndTimes.size())
				{
					laneEndTimes.push_back(0);
				}

				laneEndTimes[lane] = std::max(laneEndTimes[lane], scope->startUs + scope->durationUs);
				thread.bars.push_back({ scope, lane });
			}

			thread.laneCount = std::max<std::size_t>(laneEndTimes.size(), 1);
			threads.push_back(std::move(thread));
		}

		return threads;
	}

	int64_t TimelineDurationUs(const gns::profiling::ProfileFrameSample& frame)
	{
		int64_t durationUs = static_cast<int64_t>(frame.frameTimeMs / MicrosecondsToMilliseconds);
		for (const gns::profiling::ProfileScopeSample& scope : frame.scopes)
		{
			durationUs = std::max(durationUs, scope.startUs + scope.durationUs);
		}

		return std::max<int64_t>(durationUs, 1);
	}

	ImU32 ScopeColor(const gns::profiling::ProfileScopeSample& scope)
	{
		uint32_t hash = scope.threadId * 2166136261u;
		for (const char c : scope.name)
		{
			hash ^= static_cast<uint8_t>(c);
			hash *= 16777619u;
		}

		const float hue = static_cast<float>(hash % 360u) / 360.0f;
		return ImGui::ColorConvertFloat4ToU32(ImColor::HSV(hue, 0.62f, 0.86f));
	}

	int64_t ClampPanUs(int64_t panUs, int64_t timelineDurationUs, int64_t visibleDurationUs)
	{
		const int64_t maxPanUs = std::max<int64_t>(timelineDurationUs - visibleDurationUs, 0);
		return std::clamp(panUs, int64_t{ 0 }, maxPanUs);
	}
}

ProfilerWindow::ProfilerWindow(std::string title)
	: GuiWindow(std::move(title))
{
}

void ProfilerWindow::OnDraw()
{
	if (!m_freezeView)
	{
		RefreshFrameOverview();
		m_frames.clear();
	}

	if (m_followLatest && !m_frameTimesMs.empty())
	{
		m_selectedFrameOffsetFromLatest = 0;
	}

	DrawToolbar();
	ImGui::Separator();

	DrawFrameGraph();
	if (!m_freezeView)
	{
		return;
	}

	ImGui::Separator();
	DrawSelectedFrame();
}

void ProfilerWindow::RefreshSnapshot()
{
	m_frames = gns::profiling::Profiler::GetFrameHistory();
	m_frameOverview.clear();
	m_frameTimesMs.clear();
	m_frameTimesMs.reserve(m_frames.size());

	for (const gns::profiling::ProfileFrameSample& frame : m_frames)
	{
		m_frameTimesMs.push_back(static_cast<float>(frame.frameTimeMs));
	}
}

void ProfilerWindow::RefreshFrameOverview()
{
	m_frameOverview = gns::profiling::Profiler::GetFrameOverview();
	m_frameTimesMs.clear();
	m_frameTimesMs.reserve(m_frameOverview.size());

	for (const gns::profiling::ProfileFrameOverview& frame : m_frameOverview)
	{
		m_frameTimesMs.push_back(static_cast<float>(frame.frameTimeMs));
	}
}

void ProfilerWindow::SelectFrameByIndex(uint64_t frameIndex)
{
	if (m_frames.empty())
	{
		m_selectedFrameOffsetFromLatest = 0;
		return;
	}

	const int lastFrameIndex = static_cast<int>(m_frames.size()) - 1;
	for (int index = 0; index <= lastFrameIndex; ++index)
	{
		if (m_frames[static_cast<std::size_t>(index)].frameIndex == frameIndex)
		{
			m_selectedFrameOffsetFromLatest = lastFrameIndex - index;
			return;
		}
	}

	m_selectedFrameOffsetFromLatest = 0;
}

void ProfilerWindow::ExportTrace()
{
	if (!m_freezeView)
	{
		RefreshFrameOverview();
	}

	const std::filesystem::path outputPath = gns::path::Resolve(
		gns::path::Root::ProjectCache,
		"Profiler/GenesisEngine-Profiler.trace.json");
	if (gns::profiling::Profiler::ExportFrameHistory(outputPath, "GenesisEngine Profiler Export"))
	{
		m_exportStatus = "Exported: " + outputPath.string();
		return;
	}

	m_exportStatus = "Export failed";
}

void ProfilerWindow::DrawToolbar()
{
	const char* freezeLabel = m_freezeView ? ICON_MD_PLAY_ARROW " Resume" : ICON_MD_PAUSE " Freeze";
	if (ImGui::Button(freezeLabel))
	{
		m_freezeView = !m_freezeView;
		gns::profiling::Profiler::SetFrameCaptureEnabled(!m_freezeView);
		if (m_freezeView)
		{
			RefreshSnapshot();
		}
	}

	ImGui::SameLine();
	if (ImGui::Button(ICON_MD_SAVE " Export Trace"))
	{
		ExportTrace();
	}

	ImGui::SameLine();
	if (ImGui::Button(ICON_MD_DELETE_SWEEP " Clear"))
	{
		gns::profiling::Profiler::ClearFrameHistory();
		m_frames.clear();
		m_frameOverview.clear();
		m_frameTimesMs.clear();
		m_selectedFrameOffsetFromLatest = 0;
		m_exportStatus.clear();
	}

	ImGui::SameLine();
	ImGui::Checkbox("Follow Latest", &m_followLatest);

	ImGui::SameLine();
	ImGui::Checkbox("Table", &m_showScopeTable);

	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.0f);
	if (ImGui::InputInt("Frames", &m_frameHistoryLimit, 30, 120))
	{
		m_frameHistoryLimit = std::clamp(m_frameHistoryLimit, 0, 2000);
		gns::profiling::Profiler::SetFrameHistoryLimit(static_cast<std::size_t>(m_frameHistoryLimit));
		if (m_freezeView)
		{
			RefreshSnapshot();
		}
		m_selectedFrameOffsetFromLatest = std::min(
			m_selectedFrameOffsetFromLatest,
			std::max(0, static_cast<int>(m_frames.size()) - 1));
	}

	ImGui::SameLine();
	ImGui::Text("Captured: %d", static_cast<int>(m_frameTimesMs.size()));
	ImGui::SameLine();
	ImGui::TextUnformatted(m_freezeView ? "Paused" : "Recording");

	if (!m_exportStatus.empty())
	{
		ImGui::TextUnformatted(m_exportStatus.c_str());
	}
}

void ProfilerWindow::DrawFrameGraph()
{
	if (m_frameTimesMs.empty())
	{
		ImGui::TextUnformatted("No profiler frames captured yet.");
		return;
	}

	char overlay[96] = {};
	const int frameCount = static_cast<int>(m_frameTimesMs.size());
	const int selectedGraphIndex = std::clamp(
		frameCount - 1 - m_selectedFrameOffsetFromLatest,
		0,
		std::max(frameCount - 1, 0));

	if (m_freezeView)
	{
		const gns::profiling::ProfileFrameSample* selectedFrame =
			SelectedFrame(m_frames, m_selectedFrameOffsetFromLatest);
		if (selectedFrame != nullptr)
		{
			std::snprintf(
				overlay,
				sizeof(overlay),
				"Frame %llu: %.3f ms",
				static_cast<unsigned long long>(selectedFrame->frameIndex),
				selectedFrame->frameTimeMs);
		}
	}
	else if (!m_frameOverview.empty())
	{
		const gns::profiling::ProfileFrameOverview& selectedFrame =
			m_frameOverview[static_cast<std::size_t>(selectedGraphIndex)];
		std::snprintf(
			overlay,
			sizeof(overlay),
			"Frame %llu: %.3f ms",
			static_cast<unsigned long long>(selectedFrame.frameIndex),
			selectedFrame.frameTimeMs);
	}
	
	ImGui::PlotLines(
		"##Frame Time",
		m_frameTimesMs.data(),
		static_cast<int>(m_frameTimesMs.size()),
		0,
		overlay,
		0.0f,
		MaxFrameTime(m_frameTimesMs),
		ImVec2(ImGui::GetContentRegionAvail().x, GraphHeight));

	const ImVec2 graphMin = ImGui::GetItemRectMin();
	const ImVec2 graphMax = ImGui::GetItemRectMax();
	if (frameCount > 1)
	{
		const float graphWidth = std::max(1.0f, graphMax.x - graphMin.x);
		const float selectedX = graphMin.x +
			graphWidth * static_cast<float>(selectedGraphIndex) / static_cast<float>(frameCount - 1);
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddLine(
			ImVec2(selectedX, graphMin.y),
			ImVec2(selectedX, graphMax.y),
			ImGui::GetColorU32(ImGuiCol_PlotLinesHovered),
			2.0f);
		drawList->AddCircleFilled(
			ImVec2(selectedX, graphMin.y + 8.0f),
			4.0f,
			ImGui::GetColorU32(ImGuiCol_PlotLinesHovered));
	}

	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		const float graphWidth = std::max(1.0f, graphMax.x - graphMin.x);
		const float normalizedX = std::clamp((ImGui::GetIO().MousePos.x - graphMin.x) / graphWidth, 0.0f, 1.0f);
		const int selectedIndex = std::clamp(
			static_cast<int>(normalizedX * static_cast<float>(frameCount - 1)),
			0,
			frameCount - 1);
		uint64_t selectedFrameIndex = 0;
		bool hasSelectedFrameIndex = false;
		if (m_freezeView && selectedIndex < static_cast<int>(m_frames.size()))
		{
			selectedFrameIndex = m_frames[static_cast<std::size_t>(selectedIndex)].frameIndex;
			hasSelectedFrameIndex = true;
		}
		else if (!m_freezeView && selectedIndex < static_cast<int>(m_frameOverview.size()))
		{
			selectedFrameIndex = m_frameOverview[static_cast<std::size_t>(selectedIndex)].frameIndex;
			hasSelectedFrameIndex = true;
		}

		m_followLatest = false;
		m_freezeView = true;
		gns::profiling::Profiler::SetFrameCaptureEnabled(false);
		RefreshSnapshot();
		if (hasSelectedFrameIndex)
		{
			SelectFrameByIndex(selectedFrameIndex);
		}
		else
		{
			m_selectedFrameOffsetFromLatest = 0;
		}
		m_timelineZoom = 1.0f;
		m_timelinePanUs = 0;
	}

	if (m_freezeView && m_frames.size() > 1)
	{
		if (m_followLatest)
		{
			ImGui::BeginDisabled();
		}

		const int maxOffset = static_cast<int>(m_frames.size()) - 1;
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::SliderInt("Selected Frame", &m_selectedFrameOffsetFromLatest, 0, maxOffset, "%d frames ago");

		if (m_followLatest)
		{
			ImGui::EndDisabled();
		}
	}
}

void ProfilerWindow::DrawSelectedFrame()
{
	const gns::profiling::ProfileFrameSample* selectedFrame =
		SelectedFrame(m_frames, m_selectedFrameOffsetFromLatest);
	if (selectedFrame == nullptr)
	{
		return;
	}

	ImGui::Text(
		"Frame %llu   %.3f ms   %d scopes",
		static_cast<unsigned long long>(selectedFrame->frameIndex),
		selectedFrame->frameTimeMs,
		static_cast<int>(selectedFrame->scopes.size()));

	DrawTimeline(*selectedFrame);
	if (m_showScopeTable)
	{
		ImGui::Separator();
		DrawScopeTable(*selectedFrame);
	}
}

void ProfilerWindow::DrawTimeline(const gns::profiling::ProfileFrameSample& frame)
{
	const std::vector<TimelineThread> threads = BuildTimelineThreads(frame);
	if (threads.empty())
	{
		ImGui::TextUnformatted("No scopes captured for this frame.");
		return;
	}

	const int64_t timelineDurationUs = TimelineDurationUs(frame);
	const double timelineDurationMs = static_cast<double>(timelineDurationUs) * MicrosecondsToMilliseconds;
	const float availableWidth = ImGui::GetContentRegionAvail().x;
	m_timelineZoom = std::clamp(m_timelineZoom, 1.0f, 128.0f);
	const int64_t visibleDurationUs = std::max<int64_t>(
		static_cast<int64_t>(static_cast<double>(timelineDurationUs) / static_cast<double>(m_timelineZoom)),
		1);
	m_timelinePanUs = ClampPanUs(m_timelinePanUs, timelineDurationUs, visibleDurationUs);
	const int64_t visibleStartUs = m_timelinePanUs;
	const int64_t visibleEndUs = visibleStartUs + visibleDurationUs;
	const double visibleStartMs = static_cast<double>(visibleStartUs) * MicrosecondsToMilliseconds;
	const double visibleEndMs = static_cast<double>(visibleEndUs) * MicrosecondsToMilliseconds;
	float timelineHeight = TimelineHeaderHeight;
	for (const TimelineThread& thread : threads)
	{
		timelineHeight += TimelineLaneHeight * static_cast<float>(thread.laneCount) + TimelineThreadGap;
	}

	ImGui::Text(
		"Timeline %.3f ms   View %.3f-%.3f ms   Zoom %.1fx",
		timelineDurationMs,
		visibleStartMs,
		visibleEndMs,
		m_timelineZoom);
	
	if (ImGui::BeginChild("ProfilerTimelineScroll", ImVec2(0.0f, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_Borders))
	{
		
		ImGui::InvisibleButton("ProfilerTimelineCanvas", ImVec2(availableWidth, timelineHeight));

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 canvasMin = ImGui::GetItemRectMin();
		const ImVec2 canvasMax = ImGui::GetItemRectMax();
		const ImVec2 plotMin(canvasMin.x + TimelineLabelWidth, canvasMin.y);
		const ImVec2 plotMax(canvasMax.x, canvasMax.y);
		const float plotWidth = std::max(1.0f, plotMax.x - plotMin.x);
		const bool timelineHovered = ImGui::IsItemHovered();
		const ImGuiIO& io = ImGui::GetIO();

		if (timelineHovered && io.MouseWheel != 0.0f)
		{
			if (io.KeyShift)
			{
				const float zoomMultiplier = io.MouseWheel > 0.0f ? 1.20f : 1.0f / 1.20f;
				const float mouseTimelineT = std::clamp((io.MousePos.x - plotMin.x) / plotWidth, 0.0f, 1.0f);
				const int64_t anchorUs = visibleStartUs + static_cast<int64_t>(
					static_cast<double>(visibleDurationUs) * static_cast<double>(mouseTimelineT));

				m_timelineZoom = std::clamp(m_timelineZoom * zoomMultiplier, 1.0f, 128.0f);
				const int64_t newVisibleDurationUs = std::max<int64_t>(
					static_cast<int64_t>(static_cast<double>(timelineDurationUs) / static_cast<double>(m_timelineZoom)),
					1);
				m_timelinePanUs = anchorUs - static_cast<int64_t>(
					static_cast<double>(newVisibleDurationUs) * static_cast<double>(mouseTimelineT));
				m_timelinePanUs = ClampPanUs(m_timelinePanUs, timelineDurationUs, newVisibleDurationUs);
			}
			else if (io.KeyCtrl)
			{
				const int64_t panStepUs = std::max<int64_t>(visibleDurationUs / 10, 1);
				m_timelinePanUs = ClampPanUs(
					m_timelinePanUs - static_cast<int64_t>(io.MouseWheel) * panStepUs,
					timelineDurationUs,
					visibleDurationUs);
			}
		}

		drawList->AddRectFilled(canvasMin, canvasMax, ImGui::GetColorU32(ImGuiCol_FrameBg));
		drawList->AddLine(
			ImVec2(plotMin.x, canvasMin.y),
			ImVec2(plotMin.x, canvasMax.y),
			ImGui::GetColorU32(ImGuiCol_Border));

		for (int guideIndex = 0; guideIndex <= 4; ++guideIndex)
		{
			const double guideMs = visibleStartMs + (visibleEndMs - visibleStartMs) * static_cast<double>(guideIndex) / 4.0;
			const float x = plotMin.x + plotWidth * static_cast<float>(guideIndex) / 4.0f;
			drawList->AddLine(
				ImVec2(x, canvasMin.y),
				ImVec2(x, canvasMax.y),
				ImGui::GetColorU32(ImGuiCol_Border));

			char guideLabel[32] = {};
			std::snprintf(guideLabel, sizeof(guideLabel), "%.1fms", guideMs);
			drawList->AddText(
				ImVec2(x + 3.0f, canvasMin.y + 3.0f),
				ImGui::GetColorU32(ImGuiCol_TextDisabled),
				guideLabel);
		}

		const ImVec2 mousePos = ImGui::GetIO().MousePos;
		const gns::profiling::ProfileScopeSample* hoveredScope = nullptr;
		float rowMinY = canvasMin.y + TimelineHeaderHeight;

		for (const TimelineThread& thread : threads)
		{
			const float threadHeight = TimelineLaneHeight * static_cast<float>(thread.laneCount);
			const float rowMaxY = rowMinY + threadHeight;

			char label[32] = {};
			std::snprintf(label, sizeof(label), "Thread %u", thread.threadId);
			drawList->AddText(
				ImVec2(canvasMin.x + 6.0f, rowMinY + 4.0f),
				ImGui::GetColorU32(ImGuiCol_Text),
				label);
			drawList->AddLine(
				ImVec2(canvasMin.x, rowMaxY),
				ImVec2(canvasMax.x, rowMaxY),
				ImGui::GetColorU32(ImGuiCol_Border));

			for (const TimelineBar& bar : thread.bars)
			{
				const gns::profiling::ProfileScopeSample& scope = *bar.scope;
				const float laneY = rowMinY + TimelineLaneHeight * static_cast<float>(bar.lane);
				const float barY = laneY + (TimelineLaneHeight - TimelineBarHeight) * 0.5f;

				const int64_t scopeEndUs = scope.startUs + scope.durationUs;
				if (scopeEndUs < visibleStartUs || scope.startUs > visibleEndUs)
				{
					continue;
				}

				const float x0 = plotMin.x + plotWidth *
					static_cast<float>(scope.startUs - visibleStartUs) / static_cast<float>(visibleDurationUs);
				const float x1 = plotMin.x + plotWidth *
					static_cast<float>(scopeEndUs - visibleStartUs) / static_cast<float>(visibleDurationUs);
				const ImVec2 barMin(std::clamp(x0, plotMin.x, plotMax.x), barY);
				const ImVec2 barMax(std::clamp(std::max(x1, x0 + 1.0f), plotMin.x, plotMax.x), barY + TimelineBarHeight);

				if (barMax.x <= barMin.x)
				{
					continue;
				}

				drawList->AddRectFilled(barMin, barMax, ScopeColor(scope), 2.0f);
				if (barMax.x - barMin.x > 72.0f)
				{
					drawList->AddText(
						ImVec2(barMin.x + 4.0f, barMin.y + 1.0f),
						ImGui::GetColorU32(ImGuiCol_Text),
						scope.name.c_str());
				}

				const bool barHovered =
					timelineHovered &&
					mousePos.x >= barMin.x &&
					mousePos.x <= barMax.x &&
					mousePos.y >= barMin.y &&
					mousePos.y <= barMax.y;
				if (barHovered)
				{
					hoveredScope = &scope;
				}
			}

			rowMinY = rowMaxY + TimelineThreadGap;
		}

		if (hoveredScope != nullptr)
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(hoveredScope->name.c_str());
			ImGui::Text("Duration: %.3f ms", static_cast<double>(hoveredScope->durationUs) * MicrosecondsToMilliseconds);
			ImGui::Text("Start: %.3f ms", static_cast<double>(hoveredScope->startUs) * MicrosecondsToMilliseconds);
			ImGui::Text("Thread: %u", hoveredScope->threadId);
			ImGui::Text("%s:%u", hoveredScope->file.c_str(), hoveredScope->line);
			ImGui::EndTooltip();
		}
	}
	ImGui::EndChild();
}

void ProfilerWindow::DrawScopeTable(const gns::profiling::ProfileFrameSample& frame)
{
	constexpr ImGuiTableFlags tableFlags =
		ImGuiTableFlags_Borders |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_Reorderable |
		ImGuiTableFlags_ScrollY;

	const float tableHeight = ImGui::GetContentRegionAvail().y;
	if (tableHeight <= 0.0f)
	{
		return;
	}

	if (ImGui::BeginTable("ProfilerScopeTable", 5, tableFlags, ImVec2(0.0f, tableHeight)))
	{
		ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch, 0.42f);
		ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 88.0f);
		ImGui::TableSetupColumn("Start", ImGuiTableColumnFlags_WidthFixed, 88.0f);
		ImGui::TableSetupColumn("Thread", ImGuiTableColumnFlags_WidthFixed, 64.0f);
		ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthStretch, 0.36f);
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		const std::vector<gns::profiling::ProfileScopeSample> scopes = SortedScopes(frame);
		for (const gns::profiling::ProfileScopeSample& scope : scopes)
		{
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(scope.name.c_str());

			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%.3f ms", static_cast<double>(scope.durationUs) * MicrosecondsToMilliseconds);

			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%.3f ms", static_cast<double>(scope.startUs) * MicrosecondsToMilliseconds);

			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%u", scope.threadId);

			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%s:%u", scope.file.c_str(), scope.line);
		}

		ImGui::EndTable();
	}
}
