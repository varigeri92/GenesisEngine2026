#include "gnspch.h"
#include "Logger.h"

Logger::LogLevel Logger::s_ApplicationLogLevel = Logger::LogLevel::Trace;
std::atomic_flag Logger::s_logLock = ATOMIC_FLAG_INIT;

#ifdef ENABLE_PROFILER
namespace
{
	const char* ToProfilerLogLevel(Logger::LogLevel level)
	{
		switch (level)
		{
		case Logger::LogLevel::Trace:
			return "TRACE";
		case Logger::LogLevel::Info:
			return "INFO";
		case Logger::LogLevel::Warning:
			return "WARNING";
		case Logger::LogLevel::Error:
			return "ERROR";
		case Logger::LogLevel::Fatal:
			return "FATAL";
		default:
			return "UNKNOWN";
		}
	}
}
#endif

void Logger::LogMessage(std::string project, LogLevel level, std::string file, uint32_t line, const std::string message)
{
	if (level < s_ApplicationLogLevel)
		return;

	Lock();
#ifdef ENABLE_PROFILER
	GNS_PROFILE_LOG(ToProfilerLogLevel(level), file.c_str(), line, message);
#endif

	switch (level)
	{
	case LogLevel::Trace:
		std::cout << project << AQUA"[TRACE]: " DEFAULT << message << GRAY"(" << file << " @ line:" << line << ")\n" << DEFAULT;
		break;
	case LogLevel::Info:
		std::cout << project << BLUE"[INFO]: " DEFAULT << message << GRAY"(" << file << " @ line:" << line << ")\n" << DEFAULT;
		break;
	case LogLevel::Warning:
		std::cout << project << YELLOW"[WARNING]: "	DEFAULT << message << GRAY"(" << file << " @ line:" << line << ")\n" << DEFAULT;
		break;
	case LogLevel::Error:
		std::cout << project << RED"[ERROR]: " DEFAULT << message << GRAY"(" << file << " @ line:" << line << ")\n" << DEFAULT;
		break;
	case LogLevel::Fatal:
		std::cout << project << RED"[FATAL]: " DEFAULT << message << GRAY"(" << file << " @ line:" << line << ")\n" << DEFAULT;
		break;
	default:;
	}
	Unlock();
}
