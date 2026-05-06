#pragma once
#include "API.h"
#include <string>
#include <assert.h>
#include <cstring>

#define RED     "\x1B[31m"
#define GREEN   "\x1B[32m"
#define BLUE    "\x1B[34m"
#define YELLOW  "\x1B[33m"
#define AQUA    "\x1B[36m"
#define GRAY    "\x1B[90m"
#define DEFAULT "\x1B[0m"

#define FILE_NAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

#ifdef BUILD_DLL
#define PROJECT_SRC GREEN"[ENGINE]:"
#else
#define PROJECT_SRC GREEN"[APP]:"
#endif

class Logger
{
public:


	enum class LogLevel
	{
		Trace   = 0, 
		Info    = 1, 
		Warning = 2, 
		Error   = 3, 
		Fatal   = 4
	};
	static LogLevel s_ApplicationLogLevel;

	
	GNS_API static void LogMessage(
		std::string project, LogLevel level, std::string file, uint32_t line, const std::string message);
};
#ifdef _DEBUG
#define LOG_TRACE(message) \
	Logger::LogMessage( PROJECT_SRC,Logger::LogLevel::Trace, FILE_NAME,  __LINE__, message)
#define LOG_INFO(message) \
	Logger::LogMessage(PROJECT_SRC, Logger::LogLevel::Info, FILE_NAME,  __LINE__, message)
#define LOG_WARNING(message) \
	Logger::LogMessage(PROJECT_SRC, Logger::LogLevel::Warning, FILE_NAME,  __LINE__, message)
#define LOG_ERROR(message) \
	Logger::LogMessage(PROJECT_SRC, Logger::LogLevel::Error, FILE_NAME,  __LINE__, message)
#define LOG_FATAL(message) \
	Logger::LogMessage(PROJECT_SRC, Logger::LogLevel::Fatal, FILE_NAME,  __LINE__, message)
#define GNS_ASSERT(expr, message) \
	if (!expr){ LOG_FATAL(message);} \
	assert(expr)
#else
#define LOG_VERBOSE(msg)
#define LOG_INFO(msg)
#define LOG_WARNING(msg)
#define LOG_ERROR(msg)
#define LOG_FATAL(msg)
#define GNS_ASSERT(expr, message)
#endif


