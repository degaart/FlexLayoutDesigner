#include "LayoutView.h"

#define CLASSNAME "LayoutView"

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

bool LayoutView_Init(HINSTANCE hInstance)
{
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(wc);
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASSNAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpfnWndProc = WindowProc;
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
