#include "gnspch.h"
#include "Logger.h"

void Logger::LogMessage(std::string project, LogLevel level, std::string message, std::string file, uint32_t line)
{
	switch (level)
	{
	case LogLevel::Verbose:
		std::cout << project << AQUA"[VERBOSE]: " DEFAULT << message << GRAY"(" << file << " @ line:" << line << ")\n";
		break;
	case LogLevel::Info:
		std::cout << project << BLUE"[INFO]: " DEFAULT << message << GRAY"(" << file << " @ line:" << line << ")\n";
		break;
	case LogLevel::Warning:
		std::cout << project << YELLOW"[WARNING]: "	DEFAULT << message << GRAY"(" << file << " @ line:" << line << ")\n";
		break;
	case LogLevel::Error:
		std::cout << project << RED"[ERROR]: " DEFAULT << message << GRAY"(" << file << " @ line:" << line << ")\n";
		break;
	case LogLevel::Fatal:
		std::cout << project << RED"[FATAL]: " DEFAULT << message << GRAY"(" << file << " @ line:" << line << ")\n";
		break;
	default:;
	}
}