#include "LayoutView.h"

#include "Trace.h"
#include "Util.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CLASSNAME "LayoutView"

enum Messages
{
    LVM_SET_ROOTFLEX = WM_USER+1,
    LVM_SET_SELECTEDFLEX,
};

typedef struct WindowData
{
    YGNodeRef rootFlex;
    YGNodeRef selectedFlex;
} WindowData;

uint32_t GenerateColor(int index)
{
    if (index == 0)
    {
        return 0xFFFFFF;
    }
    return ((uint32_t)index * 2654435761u) >> 8;
}

static void PaintFlex(HDC hdc, WindowData* data, YGNodeRef flex)
{
    int index = (int)YGNodeGetContext(flex);
    uint32_t color = GenerateColor(index) & 0xFFFFFF;
    TRACE("Index=%d color=0x%X", index, color);

    float left = YGNodeLayoutGetLeft(flex);
    float top = YGNodeLayoutGetTop(flex);
    float width = YGNodeLayoutGetWidth(flex);
    float height = YGNodeLayoutGetHeight(flex);

    RECT rc;
    rc.left = roundf(left);
    rc.top = roundf(top);
    rc.right = roundf(left + width);
    rc.bottom = roundf(top + height);

    HBRUSH brush = CreateSolidBrush(color);
    if (flex == data->selectedFlex)
    {
        HPEN pen = CreatePen(PS_SOLID, 1, color ^ 0x00FFFFFF);
        SelectObject(hdc, pen);
        SelectObject(hdc, brush);
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        DeleteObject(pen);
    }
    else
    {
        FillRect(hdc, &rc, brush);
    }
    DeleteObject(brush);

    char text[16];
    snprintf(text, sizeof(text), "%d", index);
    int textLen = strlen(text);

    SetBkColor(hdc, color);
    SetTextAlign(hdc, TA_LEFT|TA_TOP);
    SetTextColor(hdc, color ^ 0x00FFFFFF);

    SIZE textSize;
    GetTextExtentPoint32(hdc, text, textLen, &textSize);

    RECT textRc;
    SetRect(&textRc, 0, 0, textSize.cx, textSize.cy);
    CenterRect(&textRc, &rc);

    ExtTextOut(hdc,
            textRc.left,
            textRc.top,
            ETO_OPAQUE,
            NULL,
            text,
            textLen,
            NULL);

    unsigned children = YGNodeGetChildCount(flex);
    for (unsigned i = 0; i < children; i++)
    {
        PaintFlex(hdc, data, YGNodeGetChild(flex, i));
    }
}

static void OnPaint(HWND hwnd, HDC hdc, PAINTSTRUCT* ps, WindowData* data)
{
    PaintFlex(hdc, data, data->rootFlex);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    WindowData* data = (WindowData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case LVM_SET_ROOTFLEX:
        if (data)
        {
            data->rootFlex = (YGNodeRef)lparam;
        }
        return 0;
    case LVM_SET_SELECTEDFLEX:
        if (data)
        {
            data->selectedFlex = (YGNodeRef)lparam;
        }
        return 0;
    case WM_PAINT:
        if (data && data->rootFlex)
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            OnPaint(hwnd, hdc, &ps, data);
            EndPaint(hwnd, &ps);
            return 0;
        }
        return 0;
    case WM_DESTROY:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0L);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

bool LayoutView_Init(HINSTANCE hInstance)
{
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(wc);
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASSNAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpfnWndProc = WindowProc;
    wc.style = CS_VREDRAW|CS_HREDRAW;
    if (!RegisterClassEx(&wc))
    {
        return false;
    }
    return true;
}

HWND LayoutView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int width, int height)
{
    HWND hwnd = CreateWindowEx(
        0,
        CLASSNAME,
        "",
        WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS,
        x, y,
        width, height,
        hParent,
        NULL,
        hInstance,
        0L);

    WindowData* data = calloc(1, sizeof(WindowData));
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)data);

    return hwnd;
}

void LayoutView_SetRootFlex(HWND hwnd, YGNodeRef rootFlex)
{
    SendMessage(hwnd, LVM_SET_ROOTFLEX, 0, (LPARAM)rootFlex);
}

void LayoutView_SetSelectedFlex(HWND hwnd, YGNodeRef flex)
{
    SendMessage(hwnd, LVM_SET_SELECTEDFLEX, 0, (LPARAM)flex);
    InvalidateRect(hwnd, NULL, FALSE);
}

