#include "CodeWindow.h"

#include "Util.h"

#include <assert.h>
#include <yoga/Yoga.h>

#define CLASSNAME "CodeWindow"

enum ControlIDs
{
    IDC_CODE_TEXT = 101,
};

enum MessageIDs
{
    CWM_SETVALUE = WM_USER + 1,
};

static void OnCreate(HWND hwnd)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    RECT rc;
    GetClientRect(hwnd, &rc);

    HWND hEdit = CreateWindowEx(
        0,
        "EDIT",
        "",
        WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|WS_BORDER|ES_MULTILINE|WS_HSCROLL|WS_VSCROLL|ES_READONLY,
        5, 5,
        rc.right - rc.left - 10, rc.bottom - rc.top - 10,
        hwnd,
        (HMENU)IDC_CODE_TEXT,
        hInstance,
        0L);

    YGNodeRef rootFlex = YGNodeNew();
    YGNodeStyleSetWidth(rootFlex, 461);
    YGNodeStyleSetHeight(rootFlex, 500);
    YGNodeStyleSetPadding(rootFlex, YGEdgeLeft, 5);
    YGNodeStyleSetPadding(rootFlex, YGEdgeTop, 5);
    YGNodeStyleSetPadding(rootFlex, YGEdgeRight, 5);
    YGNodeStyleSetPadding(rootFlex, YGEdgeBottom, 5);

    YGNodeRef editFlex = YGNodeNew();
    YGNodeStyleSetFlexGrow(editFlex, 1);

    FlexData* data = calloc(1, sizeof(FlexData));
    data->hwnd = hEdit;

    YGNodeSetContext(editFlex, data);
    YGNodeInsertChild(rootFlex, editFlex, YGNodeGetChildCount(rootFlex));

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)rootFlex);
}

static void OnSetValue(HWND hwnd, const char* value)
{
    HWND hEdit = GetDlgItem(hwnd, IDC_CODE_TEXT);
    assert(hEdit != NULL);
    SetWindowText(hEdit, value);
}

static void OnSetFont(HWND hwnd, HFONT hfont, bool redraw)
{
    HWND hEdit = GetDlgItem(hwnd, IDC_CODE_TEXT);
    if (!hEdit)
    {
        return;
    }
    SendMessage(hEdit, WM_SETFONT, (WPARAM)hfont, redraw);
}

static void OnSize(HWND hwnd, YGNodeRef rootFlex, int width, int height)
{
    YGNodeStyleSetWidth(rootFlex, width);
    YGNodeStyleSetHeight(rootFlex, height);
    YGNodeCalculateLayout(rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);
    LayoutFlex(rootFlex, 0.0f, 0.0f);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    YGNodeRef rootFlex;
    switch (msg)
    {
    case WM_CREATE:
        OnCreate(hwnd);
        return 0;
    case CWM_SETVALUE:
        OnSetValue(hwnd, (const char*)lparam);
        return 0;
    case WM_SETFONT:
        OnSetFont(hwnd, (HFONT)wparam, lparam != 0L);
        break;
    case WM_DESTROY:
        rootFlex = (YGNodeRef)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (rootFlex)
        {
            DestroyFlex(rootFlex);
        }
        break;
    case WM_SIZE:
        rootFlex = (YGNodeRef)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (rootFlex)
        {
            OnSize(hwnd, rootFlex, LOWORD(lparam), HIWORD(lparam));
        }
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

bool CodeWindow_Init(HINSTANCE hInstance)
{
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(wc);
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASSNAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpfnWndProc = WindowProc;
    if (!RegisterClassEx(&wc))
    {
        return false;
    }
    return true;
}

HWND CodeWindow_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h)
{
    HWND hwnd = CreateWindowEx(
        0,
        CLASSNAME,
        "Generated code",
        WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
        x, y,
        w, h,
        hParent,
        NULL,
        hInstance,
        0L);
    assert(hwnd != NULL);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return hwnd;
}

void CodeWindow_SetValue(HWND hControl, const char* value)
{
    SendMessage(hControl, CWM_SETVALUE, 0, (LPARAM)value);
}
