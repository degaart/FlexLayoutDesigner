#pragma once

#include <stdarg.h>

typedef struct CodeStream CodeStream;

CodeStream* CodeStream_Create();
void CodeStream_Destroy(CodeStream* stream);
void CodeStream_Write(CodeStream* stream, const char* format, ...);
const char* CodeStream_Get(CodeStream* stream);
size_t CodeStream_GetLength(CodeStream* stream);

