#pragma once

#include <stdbool.h>
#include <Windows.h>
#include <yoga/Yoga.h>

enum ValueViewNotifications
{
    VVN_CHANGED = 1,
};

typedef struct NM_VALUEVIEW
{
    NMHDR hdr;
    YGValue newValue;
} NM_VALUEVIEW;


bool ValueView_Init(HINSTANCE hInstance);
HWND ValueView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h);
void ValueView_Setvalue(HWND hwnd, YGValue value);
