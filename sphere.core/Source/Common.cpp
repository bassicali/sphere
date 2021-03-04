
#include "Common.h"

#include <cstdarg>
#include <cstdio>
#include <windows.h>

HANDLE log_file = nullptr;

void VEchoLogMessage(const char* fmt, va_list args)
{
	static char message[256];
	static char formatted[256];
	static SYSTEMTIME time = {0};
	static DWORD bytes_written = 0;

	GetSystemTime(&time);
	vsnprintf(message, 256, fmt, args);
	int len = snprintf(formatted, 256, "[%02d:%02d:%02d.%03d] %s\n", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, message);
	printf("%s", formatted);
	
	if (log_file)
	{
		WriteFile(log_file, (void*)formatted, len, &bytes_written, nullptr);
		FlushFileBuffers(log_file);
	}
}

void sphere::EchoLogMessage(const char* fmt...)
{
	va_list args;
	va_start(args, fmt);
	VEchoLogMessage(fmt, args);
	va_end(args);
}

bool sphere::CreateLogFile(const char* filename)
{
	log_file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (log_file == INVALID_HANDLE_VALUE)
	{
		log_file = nullptr;
		return false;
	}

	return true;
}
