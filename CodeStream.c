#include "CodeStream.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN 8

struct CodeStream
{
    char* buf;
    size_t capacity;
    size_t size;
};

CodeStream* CodeStream_Create()
{
    CodeStream* result = calloc(1, sizeof(CodeStream));
    return result;
}

void CodeStream_Destroy(CodeStream* stream)
{
    if (stream)
    {
        if (stream->buf)
        {
            free(stream->buf);
        }
        free(stream);
    }
}

static void CodeStream_Grow(CodeStream* stream, size_t amount)
{
    if (amount == 0)
    {
        amount = 64;
    }

    amount = ((amount + (ALIGN - 1)) / ALIGN) * ALIGN;
    stream->buf = realloc(stream->buf, stream->capacity + amount);
    stream->buf[stream->size] = '\0';
    stream->capacity += amount;
}

static bool CodeStream_TryWritev(size_t* required, CodeStream* stream, const char* format, va_list args)
{
    size_t avail = stream->capacity - stream->size;
    int ret = vsnprintf(
        stream->buf + stream->size,
        avail,
        format,
        args);
    if (ret < 0)
    {
        abort();
    }
    else if ((size_t)ret >= avail)
    {
        if (required)
        {
            *required = ret;
        }
        return false;
    }

    if (required)
    {
        *required = ret;
    }
    stream->size += ret;
    return true;
}

static void CodeStream_Writev(CodeStream* stream, const char* format, va_list args)
{
    while (true)
    {
        size_t required;
        if (CodeStream_TryWritev(&required, stream, format, args))
        {
            if (stream->capacity - stream->size < 3)
            {
                CodeStream_Grow(stream, 0);
            }
            strcat_s(stream->buf + stream->size, stream->capacity - stream->size, "\r\n");
            stream->size += 2;
            return;
        }
        CodeStream_Grow(stream, required);
    }
}

void CodeStream_Write(CodeStream* stream, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    CodeStream_Writev(stream, format, args);
    va_end(args);
}

const char* CodeStream_Get(CodeStream* stream)
{
    if (stream->size == 0)
    {
        return "";
    }
    return stream->buf;
}

size_t CodeStream_GetLength(CodeStream* stream)
{
    return stream->size;
}
