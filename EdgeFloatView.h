#pragma once

#include <stdbool.h>
#include <Windows.h>
#include <yoga\Yoga.h>

enum EdgeFloatViewNotifications
{
    EVVN_CHANGED = 300,
};

typedef struct NMEDGEFLOATVIEW
{
    NMHDR hdr;
    YGEdge edge;
    float value;
} NMEDGEFLOATVIEW;

bool EdgeFloatView_Init(HINSTANCE hInstance);
HWND EdgeFloatView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h);
void EdgeFloatView_SetValue(HWND hControl, YGEdge edge, float value);
int  EdgeFloatView_GetHeight(HWND hControl);
