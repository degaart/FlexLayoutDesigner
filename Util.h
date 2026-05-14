#pragma once

#include <stdbool.h>
#include <Windows.h>
#include <yoga\Yoga.h>

#define ENUM_STRING_CASE(V, C) \
    case C: \
        V = #C; \
        break

bool ParseFloat(const char* s, float* out);
void CenterRect(RECT* inner, const RECT* outer);
void DumpFlex(const char* title, YGNodeRef flex);

typedef struct FlexData
{
    char label[64];
    HWND hwnd;
} FlexData;

YGNodeRef CreateFlex(float width, float height, const char* name, HWND hWnd);
void DestroyFlex(YGNodeRef flex);
void LayoutFlex(YGNodeRef flex, float originX, float originY);
void SetFlexHWND(YGNodeRef flex, HWND hwnd);
uint32_t GenerateColor(int index);
int fcomp(float a, float b);
int vcomp(YGValue a, YGValue b);
uint32_t InvertColor(uint32_t color);

