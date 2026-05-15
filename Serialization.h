#pragma once

#include "Properties.h"

typedef enum SerializationError
{
    SERIALIZATION_OK,
} SerializationResult;

SerializationResult SerializeNode(const char* filename, YGNodeRef node);
SerializationResult DeserializeNode(const char* filename, YGNodeRef* node);

