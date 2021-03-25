
#pragma once

#include <string>

#define LOG_INFO(FMT,...) sphere::EchoLogMessage("[INFO] " FMT,__VA_ARGS__)
#define LOG_WARN(FMT,...) sphere::EchoLogMessage("[WARN] " FMT,__VA_ARGS__)
#define LOG_ERROR(FMT,...) sphere::EchoLogMessage("[ERRO] " FMT,__VA_ARGS__)

#define MIN(x,y) ((x) <= (y) ? (x) : (y))
#define MAX(x,y) ((x) >= (y) ? (x) : (y))

namespace sphere
{
	void EchoLogMessage(const char* fmt...);
	void SetLogFile(void* handle);
}