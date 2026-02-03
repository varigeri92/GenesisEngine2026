#pragma once
#include "API.h"
#include <string>

#define RED     "\x1B[31m"
#define GREEN   "\x1B[32m"
#define BLUE    "\x1B[34m"
#define YELLOW  "\x1B[33m"
#define AQUA    "\x1B[36m"
#define GRAY    "\x1B[90m"
#define DEFAULT "\x1B[0m"

#define FILE_NAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)



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
		Verbose, Info, Warning, Error, Fatal
	};
	GNS_API static void LogMessage(std::string project, LogLevel level, std::string message, std::string file, uint32_t line);
};
#ifdef _DEBUG
#define LOG_VERBOSE(msg) \
	Logger::LogMessage( PROJECT_SRC,Logger::LogLevel::Verbose, msg, FILE_NAME,  __LINE__)
#define LOG_INFO(msg) \
	Logger::LogMessage(PROJECT_SRC, Logger::LogLevel::Info, msg, FILE_NAME,  __LINE__)
#define LOG_WARNING(msg) \
	Logger::LogMessage(PROJECT_SRC, Logger::LogLevel::Warning, msg, FILE_NAME,  __LINE__)
#define LOG_ERROR(msg) \
	Logger::LogMessage(PROJECT_SRC, Logger::LogLevel::Error, msg, FILE_NAME,  __LINE__)
#define LOG_FATAL(msg) \
	Logger::LogMessage(PROJECT_SRC, Logger::LogLevel::Fatal, msg, FILE_NAME,  __LINE__)
#else
#define LOG_VERBOSE(msg)
#define LOG_INFO(msg)
#define LOG_WARNING(msg)
#define LOG_ERROR(msg)
#define LOG_FATAL(msg)
#endif

