#pragma once

#include "Properties.h"

typedef enum SerializationError
{
    SERIALIZATION_OK,
    SERIALIZATION_OPEN_FAILED,
    SERIALIZATION_READ_FAILED,
    SERIALIZATION_PARSE_FAILED,
    SERIALIZATION_INVALID_OBJECT
} SerializationResult;

SerializationResult SaveLayout(const char* filename, YGNodeRef node, int nextIndex);
SerializationResult LoadLayout(const char* filename, YGNodeRef* node, int* nextIndex);

