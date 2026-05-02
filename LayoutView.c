#include "LayoutView.h"

#include <yoga/Yoga.h>
#include "Trace.h"
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
    struct flex_item* rootFlex;
    struct flex_item* selectedFlex;
} WindowData;

uint32_t GenerateColor(int index)
{
    if (index == 0)
    {
        return 0xFFFFFF;
    }
    return ((uint32_t)index * 2654435761u) >> 8;
}

static void PaintFlex(HDC hdc, WindowData* data, struct flex_item* flex)
{
    int index = (uintptr_t)flex_item_get_managed_ptr(flex);
    uint32_t color = GenerateColor(index) & 0xFFFFFF;
    TRACE("Index=%d color=0x%X", index, color);

    RECT rc;
    rc.left = roundf(flex_item_get_frame_x(flex));
    rc.top = roundf(flex_item_get_frame_y(flex));

    /* 
     * If the flex has 0 children, it is not layouted, so frame_width is always 0
     * We have to hack around that
     */
    int width = roundf(flex_item_get_frame_width(flex));
    int height = roundf(flex_item_get_frame_height(flex));
    if (flex == data->rootFlex && width == 0 && height == 0)
    {
        width = roundf(flex_item_get_width(flex));
        height = roundf(flex_item_get_height(flex));
    }
    rc.right = rc.left + width;
    rc.bottom = rc.top + height;

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

    SetBkColor(hdc, color);
    SetTextAlign(hdc, TA_BASELINE|TA_CENTER);
    SetTextColor(hdc, color ^ 0x00FFFFFF);
    ExtTextOut(hdc,
            rc.left + ((rc.right - rc.left)/2),
            rc.top + ((rc.bottom - rc.top)/2),
            ETO_OPAQUE,
            NULL,
            text,
            strlen(text),
            NULL);

    unsigned children = flex_item_count(flex);
    for (unsigned i = 0; i < children; i++)
    {
        PaintFlex(hdc, data, flex_item_child(flex, i));
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
            data->rootFlex = (struct flex_item*)lparam;
        }
        return 0;
    case LVM_SET_SELECTEDFLEX:
        if (data)
        {
            data->selectedFlex = (struct flex_item*)lparam;
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
    case WM_DESTROY:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0L);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
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

void LayoutView_SetRootFlex(HWND hwnd, struct flex_item* rootFlex)
{
    SendMessage(hwnd, LVM_SET_ROOTFLEX, 0, (LPARAM)rootFlex);
}

void LayoutView_SetSelectedFlex(HWND hwnd, struct flex_item* flex)
{
    SendMessage(hwnd, LVM_SET_SELECTEDFLEX, 0, (LPARAM)flex);
    InvalidateRect(hwnd, NULL, FALSE);
}

