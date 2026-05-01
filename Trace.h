#pragma once

#define TRACE(...) __trace(__FILE__, __LINE__, __VA_ARGS__)

void __trace(const char* file, int line, const char* format, ...);
