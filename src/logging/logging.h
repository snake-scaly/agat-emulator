/*
	Agat Emulator
	Copyright (c) NOP, nnop@newmail.ru
*/

#ifndef LOGGING_H
#define LOGGING_H

typedef enum LOG_LEVEL
{
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_FATAL,
}
LOG_LEVEL;

#define LOG_FATAL(format, ...) LOG_MESSAGE(LOG_LEVEL_FATAL, format, __VA_ARGS__)

#define LOG_ERROR(format, ...) LOG_MESSAGE(LOG_LEVEL_ERROR, format, __VA_ARGS__)

#define LOG_WARN(format, ...) LOG_MESSAGE(LOG_LEVEL_WARN, format, __VA_ARGS__)

#define LOG_INFO(format, ...) LOG_MESSAGE(LOG_LEVEL_INFO, format, __VA_ARGS__)

#define LOG_DEBUG(format, ...) LOG_MESSAGE(LOG_LEVEL_DEBUG, format, __VA_ARGS__)

#define LOG_MESSAGE(level, format, ...) log_message(level, __FILE__, __LINE__, format, __VA_ARGS__)

//__attribute__((format(printf, 4, 5)))
void log_message(LOG_LEVEL level, const char* file, int line, const char* format, ...);
LOG_LEVEL get_log_level();
void set_log_level(LOG_LEVEL level);

#endif // LOGGING_H
