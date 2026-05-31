#pragma once

#include "../API/API.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace gns::profiling
{
	struct ProfileScopeSample
	{
		std::string name;
		std::string file;
		uint32_t line = 0;
		uint32_t threadId = 0;
		int64_t startUs = 0;
		int64_t durationUs = 0;
	};

	struct ProfileFrameSample
	{
		uint64_t frameIndex = 0;
		double frameTimeMs = 0.0;
		std::vector<ProfileScopeSample> scopes;
	};

	struct ProfileFrameOverview
	{
		uint64_t frameIndex = 0;
		double frameTimeMs = 0.0;
	};

	class Profiler
	{
	public:
		GNS_API static void BeginSession(const std::string& name, const std::filesystem::path& outputPath);
		GNS_API static void EndSession();
		GNS_API static void BeginFrame();
		GNS_API static void EndFrame();
		GNS_API static void ClearFrameHistory();
		GNS_API static void SetFrameHistoryLimit(std::size_t frameLimit);
		GNS_API static void SetFrameCaptureEnabled(bool enabled);
		GNS_API static bool IsFrameCaptureEnabled();
		GNS_API static std::vector<ProfileFrameSample> GetFrameHistory();
		GNS_API static std::vector<ProfileFrameOverview> GetFrameOverview();
		GNS_API static bool ExportFrameHistory(
			const std::filesystem::path& outputPath,
			const std::string& name = "GenesisEngine Profiler Export");
		GNS_API static void WriteScope(
			const char* name,
			const char* file,
			uint32_t line,
			std::chrono::steady_clock::time_point startTime,
			std::chrono::microseconds duration);
		GNS_API static void WriteLog(
			const char* level,
			const char* file,
			uint32_t line,
			const std::string& message);
		GNS_API static void SetCurrentThreadName(const char* name);
	};

	class ScopeTimer
	{
	public:
		GNS_API ScopeTimer(const char* name, const char* file, uint32_t line);
		GNS_API ~ScopeTimer();

		ScopeTimer(const ScopeTimer&) = delete;
		ScopeTimer& operator=(const ScopeTimer&) = delete;
		ScopeTimer(ScopeTimer&&) = delete;
		ScopeTimer& operator=(ScopeTimer&&) = delete;

	private:
		const char* m_name;
		const char* m_file;
		uint32_t m_line;
		std::chrono::steady_clock::time_point m_startTime;
		bool m_stopped;
	};
}

#ifdef ENABLE_PROFILER
#define GNS_PROFILE_CONCAT_IMPL(x, y) x##y
#define GNS_PROFILE_CONCAT(x, y) GNS_PROFILE_CONCAT_IMPL(x, y)
#define GNS_PROFILE_BEGIN_SESSION(name, path) ::gns::profiling::Profiler::BeginSession(name, path)
#define GNS_PROFILE_END_SESSION() ::gns::profiling::Profiler::EndSession()
#define GNS_PROFILE_BEGIN_FRAME() ::gns::profiling::Profiler::BeginFrame()
#define GNS_PROFILE_END_FRAME() ::gns::profiling::Profiler::EndFrame()
#define GNS_PROFILE_SCOPE(name) ::gns::profiling::ScopeTimer GNS_PROFILE_CONCAT(profileTimer, __LINE__)(name, __FILE__, __LINE__)
#define GNS_PROFILE_THREAD(name) ::gns::profiling::Profiler::SetCurrentThreadName(name)
#if defined(_MSC_VER)
#define GNS_PROFILE_FUNCTION() GNS_PROFILE_SCOPE(__FUNCTION__)
#elif defined(__GNUC__) || defined(__clang__)
#define GNS_PROFILE_FUNCTION() GNS_PROFILE_SCOPE(__PRETTY_FUNCTION__)
#else
#define GNS_PROFILE_FUNCTION() GNS_PROFILE_SCOPE(__FUNCTION__)
#endif
#define GNS_PROFILE_LOG(level, file, line, message) ::gns::profiling::Profiler::WriteLog(level, file, line, message)
#else
#define GNS_PROFILE_BEGIN_SESSION(name, path)
#define GNS_PROFILE_END_SESSION()
#define GNS_PROFILE_BEGIN_FRAME()
#define GNS_PROFILE_END_FRAME()
#define GNS_PROFILE_SCOPE(name)
#define GNS_PROFILE_THREAD(name)
#define GNS_PROFILE_FUNCTION()
#define GNS_PROFILE_LOG(level, file, line, message)
#endif
