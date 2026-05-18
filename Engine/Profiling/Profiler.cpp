#include "gnspch.h"
#include "Profiler.h"

#include <fstream>
#include <functional>
#include <mutex>
#include <system_error>
#include <thread>
#include <unordered_map>

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

void gns::profiling::Profiler::WriteScope(
	const char* name,
	const char* file,
	uint32_t line,
	std::chrono::steady_clock::time_point startTime,
	std::chrono::microseconds duration)
{
	std::lock_guard<std::mutex> lock(g_profilerMutex);
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
		<< "\"tid\":" << ThreadId() << ","
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
