
#pragma once

#define LOG_INFO(FMT,...) sphere::EchoLogMessage("[INFO] " FMT,__VA_ARGS__)
#define LOG_WARN(FMT,...) sphere::EchoLogMessage("[WARN] " FMT,__VA_ARGS__)
#define LOG_ERROR(FMT,...) sphere::EchoLogMessage("[ERRO] " FMT,__VA_ARGS__)

namespace sphere
{
	void EchoLogMessage(const char* fmt...);
	bool CreateLogFile(const char* filename);
}