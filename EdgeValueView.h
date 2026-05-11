#pragma once

#include <stdbool.h>
#include <Windows.h>
#include <yoga\Yoga.h>

enum EdgeValueViewNotifications
{
    EVVN_CHANGED = 200,
};

typedef struct NMEDGEVALUEVIEW
{
    NMHDR hdr;
    YGEdge edge;
    YGValue value;
} NMEDGEVALUEVIEW;

bool EdgeValueView_Init(HINSTANCE hInstance);
HWND EdgeValueView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h);
void EdgeValueView_SetValue(HWND hControl, YGEdge edge, YGValue value);
int EdgeValueView_GetHeight(HWND hControl);
