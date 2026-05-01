#include "LayoutView.h"

#include "flex/flex.h"
#include "Trace.h"
#include <stdint.h>

#define CLASSNAME "LayoutView"
#define LVM_SET_ROOTFLEX (WM_USER+1)

uint32_t GenerateColor(int index)
{
    if (index == 0)
    {
        return 0xFFFFFF;
    }
    return ((uint32_t)index * 2654435761u) >> 8;
}

static void PaintFlex(HDC hdc, struct flex_item* flex)
{
    int index = (uintptr_t)flex_item_get_managed_ptr(flex);
    uint32_t color = GenerateColor(index);
    TRACE("Index=%d color=0x%X", index, color);

    RECT rc;
    rc.left = flex_item_get_frame_x(flex);
    rc.top = flex_item_get_frame_y(flex);

    /* 
     * If the flex has 0 children, it is not layouted, so frame_width is always 0
     * We have to hack around that
     */
    int width = flex_item_get_frame_width(flex);
    if (width == 0)
    {
        width = flex_item_get_width(flex);
    }

    int height = flex_item_get_frame_height(flex);
    if (height == 0)
    {
        height = flex_item_get_height(flex);
    }

    rc.right = rc.left + width;
    rc.bottom = rc.top + height;

    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rc, brush);
    DeleteObject(brush);

    unsigned children = flex_item_count(flex);
    for (unsigned i = 0; i < children; i++)
    {
        PaintFlex(hdc, flex_item_child(flex, i));
    }
}

static void OnSetRootFlex(HWND hwnd, struct flex_item* rootFlex)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)rootFlex);
}

static void OnPaint(HWND hwnd, HDC hdc, PAINTSTRUCT* ps, struct flex_item* rootFlex)
{
    PaintFlex(hdc, rootFlex);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case LVM_SET_ROOTFLEX:
        OnSetRootFlex(hwnd, (struct flex_item*)lparam);
        break;
    case WM_PAINT:
        {
            struct flex_item* rootFlex = (struct flex_item*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            OnPaint(hwnd, hdc, &ps, rootFlex);
            EndPaint(hwnd, &ps);
            return 0;
        }
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

    return hwnd;
}

void LayoutView_SetRootFlex(HWND hwnd, struct flex_item* rootFlex)
{
    SendMessage(hwnd, LVM_SET_ROOTFLEX, 0, (LPARAM)rootFlex);
}

