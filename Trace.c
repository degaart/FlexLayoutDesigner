#include "Trace.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <Shlwapi.h>

void __trace(const char* file, int line, const char* format, ...)
{
    char* filename = strdup(file);
    PathStripPath(filename);

    char buffer[1024];
    int curLen = snprintf(buffer, 32, "[%s:%d] ", filename, line);
    free(filename);

    if (curLen < 0 || curLen > sizeof(buffer) - 1)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(buffer+curLen, sizeof(buffer) - curLen - 2, format, args);
    va_end(args);

    strcat(buffer, "\r\n");

    OutputDebugString(buffer);
}
