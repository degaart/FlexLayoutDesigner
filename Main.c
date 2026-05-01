/*
 *
 * ┌──────────────────────┐ ┌─────────────────────────────────┐
 * │         Add          │ │                                 │
 * └──────────────────────┘ │                                 │
 * ┌──────────────────────┐ │                                 │
 * │    Generate Code     │ │                                 │
 * └──────────────────────┘ │                                 │
 * ┌─Layout───────────────┐ │                                 │
 * │ + root               │ │                                 │
 * │   + timerLabel       │ │        LAYOUT PREVIEW           │
 * │   + buttonContainer  │ │                                 │
 * │     + startButton    │ │                                 │
 * │     + stopButton     │ │                                 │
 * │     + resetButton    │ │                                 │
 * └──────────────────────┘ │                                 │
 * ┌─Properties───────────┐ │                                 │
 * │ Width:         [   ] │ │                                 │
 * │ Height:        [   ] │ │                                 │
 * │ Align:  [ CENTER  v] │ │                                 │
 * │ Wrap:   [ NO_WRAP v] │ │                                 │
 * └──────────────────────┘ └─────────────────────────────────┘
 *
*/
#include <Windows.h>
#include <stdbool.h>
#include "flex/flex.h"
#include "LayoutView.h"
#include "ScrollView.h"
#include <CommCtrl.h>
#include <stdio.h>

#define APPNAME "FlexLayoutDesigner"

typedef struct AppState
{
    HINSTANCE hInstance;
    HFONT hFont;
    HWND hLayoutView;
} AppState;

typedef enum PropertyType
{
    PROPERTY_TYPE_FLOAT,
    PROPERTY_TYPE_INT,

    PROPERTY_TYPE_ALIGN,
    PROPERTY_TYPE_POSITION,
    PROPERTY_TYPE_DIRECTION,
    PROPERTY_TYPE_WRAP,
} PropertyType;

typedef struct Property
{
    const char* name;
    PropertyType type;
} Property;

static void CreateProperties(HINSTANCE hInstance, HWND hParent)
{
    RECT rc;
    GetClientRect(hParent, &rc);

    Property properties[] =
    {
        {"width", PROPERTY_TYPE_FLOAT},
        {"height", PROPERTY_TYPE_FLOAT},
        {"justify_content", PROPERTY_TYPE_ALIGN},
        {"position", PROPERTY_TYPE_POSITION},
        {"direction", PROPERTY_TYPE_DIRECTION},
        {"wrap", PROPERTY_TYPE_WRAP},
        {"order", PROPERTY_TYPE_INT},
        {NULL, 0},
    };

    int x = 10;
    int y = 10;
    for (const Property* prop = properties; prop->name; prop++)
    {
        char label[128];
        snprintf(label, sizeof(label), "%s:", prop->name);
        HWND hLabel = CreateWindow(
            "STATIC",
            label,
            WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
            x, y,
            90, 32,
            hParent,
            NULL,
            hInstance,
            0L);

        const char* windowClass = NULL;
        unsigned style = 0;
        int h = 32;
        switch (prop->type)
        {
        case PROPERTY_TYPE_FLOAT:
        case PROPERTY_TYPE_INT:
            windowClass = "EDIT";
            style = WS_BORDER;
            h = 24;
            break;
        case PROPERTY_TYPE_ALIGN:
        case PROPERTY_TYPE_POSITION:
        case PROPERTY_TYPE_DIRECTION:
        case PROPERTY_TYPE_WRAP:
        default:
            windowClass = "COMBOBOX";
            style = CBS_DROPDOWN;
            break;
        }

        HWND hwnd = CreateWindow(
            windowClass,
            "",
            WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|style,
            100, y,
            rc.right - rc.left - 90 - x - 10, h,
            hParent,
            NULL,
            hInstance,
            0L);
        if (!hwnd)
        {
            MessageBox(hParent, "Failed to create control", "Error", MB_ICONERROR | MB_OK);
            ExitProcess(1);
        }
        y += 32 + 10;
    }
}

static void ApplyFont(HWND hwnd, HFONT hfont)
{
    SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, TRUE);

    for (HWND hChild = GetWindow(hwnd, GW_CHILD); hChild; hChild = GetWindow(hChild, GW_HWNDNEXT))
    {
        ApplyFont(hChild, hfont);
    }
}

static bool OnCreate(HWND hwnd, AppState* appState)
{
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)appState);

    RECT rc;
    GetClientRect(hwnd, &rc);

    HWND addButton = CreateWindow(
        "BUTTON",
        "Add",
        WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS,
        0, 0,
        300, 32,
        hwnd,
        NULL,
        appState->hInstance,
        0L);

    HWND generateCodeButton = CreateWindow(
        "BUTTON",
        "Generate Code",
        WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS,
        0, 32+10,
        300, 32,
        hwnd,
        NULL,
        appState->hInstance,
        0L);

    HWND layoutTreeView = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_TREEVIEW,
        "",
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
        10, 32 + 10 + 32 + 20 + 20,
        280, 170,
        hwnd,
        NULL,
        appState->hInstance,
        0L);

    HWND layoutGroupBox = CreateWindow(
        "BUTTON",
        "Layout",
        WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|BS_GROUPBOX,
        0, 32+10+32+20,
        300, 200,
        hwnd,
        NULL,
        appState->hInstance,
        0L);

    HWND propertiesContainer = ScrollView_Create(appState->hInstance,
                                                 hwnd,
                                                 10, 32 + 10 + 32 + 10 + 200 + 20 + 20,
                                                 280, 170);

    CreateProperties(appState->hInstance, propertiesContainer);
    SendMessage(propertiesContainer, SVM_UPDATESCROLL, 0, 0);

    HWND propertiesGroupBox = CreateWindow(
        "BUTTON",
        "Properties",
        WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|BS_GROUPBOX,
        0, 32+10+32+10+200+20,
        300, 200,
        hwnd,
        NULL,
        appState->hInstance,
        0L);

    appState->hLayoutView = LayoutView_Create(appState->hInstance,
                                              hwnd,
                                              310, 10,
                                              rc.right - rc.left - 200 - 20, rc.bottom - rc.top - 20);

    NONCLIENTMETRICS ncm = {sizeof(ncm)};
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

    appState->hFont = CreateFontIndirect(&ncm.lfMessageFont);
    ApplyFont(hwnd, appState->hFont);

    return true;
}

static void OnSize(AppState* appState, HWND hwnd, WORD width, WORD height)
{
    RECT rc;
    GetWindowRect(appState->hLayoutView, &rc);
    MapWindowPoints(NULL, hwnd, (LPPOINT)&rc, 2);

    int layoutViewWidth = rc.left - rc.right;
    if (width > rc.left)
    {
        layoutViewWidth = width - rc.left - 10;
    }

    int layoutViewHeight = rc.bottom - rc.top;
    if (height > rc.top)
    {
        layoutViewHeight = height - rc.top - 10;
    }

    SetWindowPos(appState->hLayoutView,
        NULL,
        rc.left, rc.right,
        layoutViewWidth, layoutViewHeight,
        SWP_NOMOVE|SWP_NOZORDER);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    AppState* appState;
    switch (msg)
    {
    case WM_CREATE:
        if (OnCreate(hwnd, ((CREATESTRUCTA*)lparam)->lpCreateParams))
        {
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_SIZE:
        appState = (AppState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (appState)
        {
            OnSize(appState, hwnd, LOWORD(lparam), HIWORD(lparam));
        }
        return 0;
    case WM_DESTROY:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static bool InitApplication(HINSTANCE hInstance)
{
    INITCOMMONCONTROLSEX ics = {
        .dwSize = sizeof(INITCOMMONCONTROLSEX),
        .dwICC = ICC_TREEVIEW_CLASSES
    };
    if (!InitCommonControlsEx(&ics))
    {
        return false;
    }

    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(wc);
    wc.hInstance = hInstance;
    wc.lpszClassName = APPNAME;
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpfnWndProc = WindowProc;
    if (!RegisterClassExA(&wc))
    {
        return false;
    }

    if (!LayoutView_Init(hInstance))
    {
        return false;
    }

    if (!ScrollView_Init(hInstance))
    {
        return false;
    }

    return true;
}

static bool InitInstance(HINSTANCE hInstance, INT nShowCmd, AppState* appState)
{
    unsigned style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;

    RECT rcWindow = {
        .top = 0,
        .left = 0,
        .right = 640,
        .bottom = 520,
    };
    AdjustWindowRectEx(&rcWindow, style, FALSE, 0);

    HWND hwnd = CreateWindowA(
        APPNAME,
        APPNAME,
        style,
        CW_USEDEFAULT, 0,
        rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top,
        NULL,
        NULL,
        hInstance,
        appState);
    if (!hwnd)
    {
        return false;
    }
    ShowWindow(hwnd, nShowCmd);
    UpdateWindow(hwnd);
    return true;
}

INT WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   INT nShowCmd)
{
    AppState appState = {0};
    appState.hInstance = hInstance;

    if (!InitApplication(hInstance))
    {
        return 1;
    }
    else if (!InitInstance(hInstance, nShowCmd, &appState))
    {
        return 1;
    }

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}
