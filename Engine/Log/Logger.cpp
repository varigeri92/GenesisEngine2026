#include "gnspch.h"
#include "Logger.h"

Logger::LogLevel Logger::s_ApplicationLogLevel = Logger::LogLevel::Trace;

void Logger::LogMessage(std::string project, LogLevel level, std::string file, uint32_t line, const std::string message)
{
	if (level < s_ApplicationLogLevel)
		return;

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
}