#include "GroupBox.h"

#include <CommCtrl.h>

#define SUBCLASSID_GROUPBOX 0

static LRESULT CALLBACK GroupBoxSubclassProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR uid, DWORD_PTR data)
{
    if (msg == WM_ERASEBKGND)
    {
        HDC hdc = (HDC)wparam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE + 1));
        return 1;
    }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

bool GroupBox_Init(HINSTANCE hInstance)
{
    return true;
}

HWND GroupBox_Create(HINSTANCE hInstance, HWND hParent,
        const char* title,
        int x, int y, int w, int h)
{
    HWND hwnd = CreateWindow(
        "BUTTON",
        title,
        WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|BS_GROUPBOX,
        x, y,
        w, h,
        hParent,
        NULL,
        hInstance,
        0L);
    SetWindowSubclass(hwnd, GroupBoxSubclassProc, SUBCLASSID_GROUPBOX, 0);
    return hwnd;
}

