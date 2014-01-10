#include "logger.h"

void Log(const char *format, ...) {
    va_list args;

    va_start(args, format);
    vfprintf(stdout, format, args);
    fflush(stdout);
    va_end(args);
}
