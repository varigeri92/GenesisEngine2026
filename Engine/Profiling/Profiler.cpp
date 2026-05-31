#include "gnspch.h"
#include "Profiler.h"

#include <cstddef>
#include <fstream>
#include <functional>
#include <mutex>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
	struct ProfileSession
	{
		std::string name;
		std::ofstream stream;
		std::chrono::steady_clock::time_point startTime;
		bool firstEvent = true;
	};

	std::mutex g_profilerMutex;
	ProfileSession g_session;
	bool g_sessionActive = false;
	std::unordered_map<std::thread::id, uint32_t> g_threadIds;
	uint32_t g_nextThreadId = 0;
	std::size_t g_frameHistoryLimit = 300;
	uint64_t g_nextFrameIndex = 0;
	bool g_frameActive = false;
	bool g_frameCaptureEnabled = true;
	gns::profiling::ProfileFrameSample g_currentFrame;
	std::chrono::steady_clock::time_point g_currentFrameStartTime;
	std::vector<gns::profiling::ProfileFrameSample> g_frameHistory;
	std::size_t g_frameHistoryStart = 0;
	std::size_t g_frameHistoryCount = 0;

	std::string JsonEscape(const std::string& value)
	{
		std::string escaped;
		escaped.reserve(value.size());

		for (const char c : value)
		{
			switch (c)
			{
			case '\\':
				escaped += "\\\\";
				break;
			case '"':
				escaped += "\\\"";
				break;
			case '\n':
				escaped += "\\n";
				break;
			case '\r':
				escaped += "\\r";
				break;
			case '\t':
				escaped += "\\t";
				break;
			default:
				escaped += c;
				break;
			}
		}

		return escaped;
	}

	uint32_t ThreadId()
	{
		const std::thread::id threadId = std::this_thread::get_id();
		if (const auto it = g_threadIds.find(threadId); it != g_threadIds.end())
		{
			return it->second;
		}

		const uint32_t profilerThreadId = g_nextThreadId++;
		g_threadIds[threadId] = profilerThreadId;
		return profilerThreadId;
	}

	int64_t MicrosecondsSinceSessionStart(std::chrono::steady_clock::time_point timePoint)
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(timePoint - g_session.startTime).count();
	}

	void BeginEvent()
	{
		if (!g_session.firstEvent)
		{
			g_session.stream << ",\n";
			return;
		}

		g_session.firstEvent = false;
	}

	void WriteStringArg(const char* key, const std::string& value)
	{
		g_session.stream << "\"" << key << "\":\"" << JsonEscape(value) << "\"";
	}

	void CloseSession()
	{
		if (!g_sessionActive)
			return;

		g_session.stream << "\n]}";
		g_session.stream.close();
		g_session = {};
		g_sessionActive = false;
	}

	void PushFrameToHistory(gns::profiling::ProfileFrameSample frame)
	{
		if (g_frameHistoryLimit == 0)
			return;

		if (g_frameHistory.size() < g_frameHistoryLimit)
		{
			g_frameHistory.push_back(std::move(frame));
			g_frameHistoryCount = g_frameHistory.size();
			return;
		}

		g_frameHistory[g_frameHistoryStart] = std::move(frame);
		g_frameHistoryStart = (g_frameHistoryStart + 1) % g_frameHistoryLimit;
		g_frameHistoryCount = g_frameHistoryLimit;
	}

	std::vector<gns::profiling::ProfileFrameSample> GetFrameHistoryInChronologicalOrder()
	{
		std::vector<gns::profiling::ProfileFrameSample> frames;
		frames.reserve(g_frameHistoryCount);

		for (std::size_t i = 0; i < g_frameHistoryCount; ++i)
		{
			const std::size_t index = (g_frameHistoryStart + i) % g_frameHistory.size();
			frames.push_back(g_frameHistory[index]);
		}

		return frames;
	}

	std::vector<gns::profiling::ProfileFrameOverview> GetFrameOverviewInChronologicalOrder()
	{
		std::vector<gns::profiling::ProfileFrameOverview> frames;
		frames.reserve(g_frameHistoryCount);

		for (std::size_t i = 0; i < g_frameHistoryCount; ++i)
		{
			const std::size_t index = (g_frameHistoryStart + i) % g_frameHistory.size();
			frames.push_back({ g_frameHistory[index].frameIndex, g_frameHistory[index].frameTimeMs });
		}

		return frames;
	}
}

void gns::profiling::Profiler::BeginSession(const std::string& name, const std::filesystem::path& outputPath)
{
	std::lock_guard<std::mutex> lock(g_profilerMutex);

	CloseSession();

	if (outputPath.has_parent_path())
	{
		std::error_code error;
		std::filesystem::create_directories(outputPath.parent_path(), error);
	}

	g_session.name = name;
	g_session.startTime = std::chrono::steady_clock::now();
	g_threadIds.clear();
	g_nextThreadId = 0;
	g_session.stream.open(outputPath);
	if (!g_session.stream.is_open())
	{
		g_session = {};
		return;
	}

	g_sessionActive = true;
	g_session.stream << "{\"traceEvents\":[\n";
}

void gns::profiling::Profiler::EndSession()
{
	std::lock_guard<std::mutex> lock(g_profilerMutex);
	CloseSession();
}

void gns::profiling::Profiler::BeginFrame()
{
	std::lock_guard<std::mutex> lock(g_profilerMutex);
	if (!g_frameCaptureEnabled)
	{
		g_frameActive = false;
		return;
	}

	g_currentFrame = {};
	g_currentFrame.frameIndex = g_nextFrameIndex++;
	g_currentFrameStartTime = std::chrono::steady_clock::now();
	g_frameActive = true;
}

void gns::profiling::Profiler::EndFrame()
{
	std::lock_guard<std::mutex> lock(g_profilerMutex);
	if (!g_frameActive)
		return;

	const auto endTime = std::chrono::steady_clock::now();
	g_currentFrame.frameTimeMs = std::chrono::duration<double, std::milli>(endTime - g_currentFrameStartTime).count();
	PushFrameToHistory(std::move(g_currentFrame));

	g_currentFrame = {};
	g_frameActive = false;
}

void gns::profiling::Profiler::ClearFrameHistory()
{
	std::lock_guard<std::mutex> lock(g_profilerMutex);
	g_frameHistory.clear();
	g_frameHistoryStart = 0;
	g_frameHistoryCount = 0;
}

void gns::profiling::Profiler::SetFrameHistoryLimit(std::size_t frameLimit)
{
	std::lock_guard<std::mutex> lock(g_profilerMutex);

	std::vector<gns::profiling::ProfileFrameSample> frames = GetFrameHistoryInChronologicalOrder();
	g_frameHistoryLimit = frameLimit;

	if (frames.size() > g_frameHistoryLimit)
	{
		frames.erase(frames.begin(), frames.begin() + static_cast<std::ptrdiff_t>(frames.size() - g_frameHistoryLimit));
	}

	g_frameHistory = std::move(frames);
	g_frameHistoryStart = 0;
	g_frameHistoryCount = g_frameHistory.size();
}

void gns::profiling::Profiler::SetFrameCaptureEnabled(bool enabled)
{
	std::lock_guard<std::mutex> lock(g_profilerMutex);
	g_frameCaptureEnabled = enabled;
	if (!g_frameCaptureEnabled)
	{
		g_currentFrame = {};
		g_frameActive = false;
	}
}

bool gns::profiling::Profiler::IsFrameCaptureEnabled()
{
	std::lock_guard<std::mutex> lock(g_profilerMutex);
	return g_frameCaptureEnabled;
}

std::vector<gns::profiling::ProfileFrameSample> gns::profiling::Profiler::GetFrameHistory()
{
	std::lock_guard<std::mutex> lock(g_profilerMutex);
	return GetFrameHistoryInChronologicalOrder();
}

std::vector<gns::profiling::ProfileFrameOverview> gns::profiling::Profiler::GetFrameOverview()
{
	std::lock_guard<std::mutex> lock(g_profilerMutex);
	return GetFrameOverviewInChronologicalOrder();
}

bool gns::profiling::Profiler::ExportFrameHistory(
	const std::filesystem::path& outputPath,
	const std::string& name)
{
	const std::vector<ProfileFrameSample> frames = GetFrameHistory();

	if (outputPath.has_parent_path())
	{
		std::error_code error;
		std::filesystem::create_directories(outputPath.parent_path(), error);
		if (error)
		{
			return false;
		}
	}

	std::ofstream stream(outputPath);
	if (!stream.is_open())
	{
		return false;
	}

	stream << "{\"traceEvents\":[\n";
	bool firstEvent = true;
	int64_t frameStartUs = 0;

	const auto beginTraceEvent = [&]()
	{
		if (!firstEvent)
		{
			stream << ",\n";
			return;
		}

		firstEvent = false;
	};

	beginTraceEvent();
	stream
		<< "{\"cat\":\"metadata\","
		<< "\"name\":\"process_name\","
		<< "\"ph\":\"M\","
		<< "\"pid\":0,"
		<< "\"tid\":0,"
		<< "\"args\":{\"name\":\"" << JsonEscape(name) << "\"}}";

	for (const ProfileFrameSample& frame : frames)
	{
		beginTraceEvent();
		stream
			<< "{\"cat\":\"frame\","
			<< "\"name\":\"Frame " << frame.frameIndex << "\","
			<< "\"ph\":\"X\","
			<< "\"ts\":" << frameStartUs << ","
			<< "\"dur\":" << static_cast<int64_t>(frame.frameTimeMs * 1000.0) << ","
			<< "\"pid\":0,"
			<< "\"tid\":0,"
			<< "\"args\":{\"frameIndex\":" << frame.frameIndex << "}}";

		for (const ProfileScopeSample& scope : frame.scopes)
		{
			beginTraceEvent();
			stream
				<< "{\"cat\":\"scope\","
				<< "\"name\":\"" << JsonEscape(scope.name) << "\","
				<< "\"ph\":\"X\","
				<< "\"ts\":" << frameStartUs + scope.startUs << ","
				<< "\"dur\":" << scope.durationUs << ","
				<< "\"pid\":0,"
				<< "\"tid\":" << scope.threadId << ","
				<< "\"args\":{";
			stream << "\"file\":\"" << JsonEscape(scope.file) << "\"";
			stream << ",\"line\":" << scope.line << ",\"frameIndex\":" << frame.frameIndex << "}}";
		}

		frameStartUs += static_cast<int64_t>(frame.frameTimeMs * 1000.0);
	}

	stream << "\n]}";
	return true;
}

void gns::profiling::Profiler::WriteScope(
	const char* name,
	const char* file,
	uint32_t line,
	std::chrono::steady_clock::time_point startTime,
	std::chrono::microseconds duration)
{
	std::lock_guard<std::mutex> lock(g_profilerMutex);
	const uint32_t threadId = ThreadId();

	if (g_frameActive)
	{
		gns::profiling::ProfileScopeSample sample;
		sample.name = name != nullptr ? name : "Scope";
		sample.file = file != nullptr ? file : "";
		sample.line = line;
		sample.threadId = threadId;
		sample.startUs = std::chrono::duration_cast<std::chrono::microseconds>(startTime - g_currentFrameStartTime).count();
		sample.durationUs = duration.count();
		g_currentFrame.scopes.push_back(std::move(sample));
	}

	if (!g_sessionActive)
		return;

	BeginEvent();

	g_session.stream
		<< "{\"cat\":\"scope\","
		<< "\"name\":\"" << JsonEscape(name != nullptr ? name : "Scope") << "\","
		<< "\"ph\":\"X\","
		<< "\"ts\":" << MicrosecondsSinceSessionStart(startTime) << ","
		<< "\"dur\":" << duration.count() << ","
		<< "\"pid\":0,"
		<< "\"tid\":" << threadId << ","
		<< "\"args\":{";
	WriteStringArg("file", file != nullptr ? file : "");
	g_session.stream << ",\"line\":" << line << "}}";
}

void gns::profiling::Profiler::WriteLog(const char* level, const char* file, uint32_t line, const std::string& message)
{
	std::lock_guard<std::mutex> lock(g_profilerMutex);
	if (!g_sessionActive)
		return;

	BeginEvent();

	g_session.stream
		<< "{\"cat\":\"log\","
		<< "\"name\":\"LOG_" << JsonEscape(level != nullptr ? level : "UNKNOWN") << "\","
		<< "\"ph\":\"i\","
		<< "\"s\":\"t\","
		<< "\"ts\":" << MicrosecondsSinceSessionStart(std::chrono::steady_clock::now()) << ","
		<< "\"pid\":0,"
		<< "\"tid\":" << ThreadId() << ","
		<< "\"args\":{";
	WriteStringArg("level", level != nullptr ? level : "UNKNOWN");
	g_session.stream << ",";
	WriteStringArg("file", file != nullptr ? file : "");
	g_session.stream << ",\"line\":" << line << ",";
	WriteStringArg("message", message);
	g_session.stream << "}}";
}

void gns::profiling::Profiler::SetCurrentThreadName(const char* name)
{
	std::lock_guard<std::mutex> lock(g_profilerMutex);
	if (!g_sessionActive)
		return;

	BeginEvent();

	g_session.stream
		<< "{\"cat\":\"metadata\","
		<< "\"name\":\"thread_name\","
		<< "\"ph\":\"M\","
		<< "\"pid\":0,"
		<< "\"tid\":" << ThreadId() << ","
		<< "\"args\":{";
	WriteStringArg("name", name != nullptr ? name : "Thread");
	g_session.stream << "}}";
}

gns::profiling::ScopeTimer::ScopeTimer(const char* name, const char* file, uint32_t line)
	: m_name(name),
	  m_file(file),
	  m_line(line),
	  m_startTime(std::chrono::steady_clock::now()),
	  m_stopped(false)
{
}

gns::profiling::ScopeTimer::~ScopeTimer()
{
	if (m_stopped)
		return;

	const auto endTime = std::chrono::steady_clock::now();
	const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_startTime);
	Profiler::WriteScope(m_name, m_file, m_line, m_startTime, duration);
	m_stopped = true;
}
