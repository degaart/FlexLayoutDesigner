#pragma once

#include "Properties.h"

#include <stdbool.h>
#include <Windows.h>
#include <yoga\Yoga.h>

#define ENUM_STRING_CASE(V, C) \
    case C: \
        V = #C; \
        break

#define UNHANDLED_CASE() \
    default: \
        assert(!"Unhandled case value"); \
        break

bool ParseFloat(const char* s, float* out);
void CenterRect(RECT* inner, const RECT* outer);
void DumpFlex(const char* title, YGNodeRef flex);

typedef struct FlexData
{
    HWND hwnd;
} FlexData;

YGNodeRef CreateFlex(float width, float height, HWND hWnd);
void DestroyFlex(YGNodeRef flex);
void LayoutFlex(YGNodeRef flex, float originX, float originY);
void SetFlexHWND(YGNodeRef flex, HWND hwnd);
uint32_t GenerateColor(int index);
int fcomp(float a, float b);
int vcomp(YGValue a, YGValue b);
uint32_t InvertColor(uint32_t color);
bool OpenFileDialog(HWND hParent, char* buffer, size_t bufferSize);
bool SaveFileDialog(HWND hParent, char* buffer, size_t bufferSize);

struct EnumPropertiesParams;
typedef struct EnumPropertiesParams EnumPropertiesParams;

struct EnumPropertiesParams
{
    void (*onFloat)(EnumPropertiesParams*, const Property*, YGNodeConstRef, float);
    void (*onValue)(EnumPropertiesParams*, const Property*, YGNodeConstRef, YGValue);
    void (*onAlign)(EnumPropertiesParams*, const Property*, YGNodeConstRef, YGAlign, const char*);
    void (*onPosition)(EnumPropertiesParams*, const Property*, YGNodeConstRef, YGPositionType, const char*);
    void (*onDirection)(EnumPropertiesParams*, const Property*, YGNodeConstRef, YGFlexDirection, const char*);
    void (*onWrap)(EnumPropertiesParams*, const Property*, YGNodeConstRef, YGWrap, const char*);
    void (*onEdgeValue)(EnumPropertiesParams*, const Property*, YGNodeConstRef, YGEdge, const char*, YGValue);
    void (*onEdgeFloat)(EnumPropertiesParams*, const Property*, YGNodeConstRef, YGEdge, const char*, float);
    void (*onJustify)(EnumPropertiesParams*, const Property*, YGNodeConstRef, YGJustify, const char*);
    void (*onOverflow)(EnumPropertiesParams*, const Property*, YGNodeConstRef, YGOverflow, const char*);
    void (*onDisplay)(EnumPropertiesParams*, const Property*, YGNodeConstRef, YGDisplay, const char*);
    void (*onString)(EnumPropertiesParams*, const Property*, YGNodeConstRef, const char*);

    void (*onChildNode)(EnumPropertiesParams*, YGNodeConstRef, YGNodeConstRef);
    void* ctx;
};
void EnumNodeProperties(YGNodeConstRef node, EnumPropertiesParams* params, bool ignoreDefault);

