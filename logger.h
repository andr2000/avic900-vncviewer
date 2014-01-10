#ifndef _LOGGER_H
#define _LOGGER_H

#include <stdio.h>

#if(defined __cplusplus)
extern "C"
{
#endif

#define LOG_APP_NAME "vncviewer"

#ifdef RELEASE
#define _LOG(log_level, fmt, ...)
#else
#define _LOG(log_level, fmt, ...)						\
  do {													\
	Log(LOG_APP_NAME #log_level ": (%s:%d): " fmt "\n",	\
            __FUNCTION__, __LINE__ , ## __VA_ARGS__);	\
  } while (0)
#endif
 
#define LOG(fmt, ...) do _LOG(fmt, ## __VA_ARGS__); while (0)

void Log(const char *format, ...);

#if(defined __cplusplus)
}
#endif

#endif