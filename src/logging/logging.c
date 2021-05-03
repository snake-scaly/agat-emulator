/*
	Agat Emulator
	Copyright (c) NOP, nnop@newmail.ru
*/

#include "logging.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static volatile LOG_LEVEL global_log_level = LOG_LEVEL_INFO;

static const char* log_level_names[] =
{
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
	"FATAL",
};

static const char* basename(const char* path)
{
	const char* fwd = strrchr(path, '/');
	const char* bak = strrchr(path, '\\');
	const char* sep = fwd > bak ? fwd : bak;
	return sep ? sep + 1 : path;
}

void log_message(LOG_LEVEL level, const char* file, int line, const char* format, ...)
{
	va_list ap;

	if (level < global_log_level) {
		return;
	}

	if (level >= 0 && level < sizeof(log_level_names) / sizeof(*log_level_names)) {
		printf("[%.5s] ", log_level_names[level]);
	} else {
		printf("[%5d] ", level);
	}

	printf("%s:%d: ", basename(file), line);

	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);

	printf("\n");
	fflush(stdout);
}

LOG_LEVEL get_log_level()
{
	return global_log_level;
}

void set_log_level(LOG_LEVEL level)
{
	global_log_level = level;
}
