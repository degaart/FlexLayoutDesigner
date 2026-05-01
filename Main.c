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
#include "GroupBox.h"
#include <CommCtrl.h>
#include <stdio.h>

#define APPNAME "FlexLayoutDesigner"

enum ButtonIDs
{
    ID_ADDBUTTON = 101,
    ID_GENERATEBUTTON,
};

typedef struct AppState
{
    HINSTANCE hInstance;
    HFONT hFont;
    HWND hLayoutView;
    HWND hLayoutTree;
    HTREEITEM hRootTreeItem;
    int index;
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

static void OnCreate(HWND hwnd, AppState* appState)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)appState);

    RECT rc;
    GetClientRect(hwnd, &rc);

    HWND addButton = CreateWindow(
        "BUTTON",
        "Add",
        WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS,
        0, 0,
        300, 32,
        hwnd,
        (HMENU)ID_ADDBUTTON,
        appState->hInstance,
        0L);

    HWND generateCodeButton = CreateWindow(
        "BUTTON",
        "Generate Code",
        WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS,
        0, 32+10,
        300, 32,
        hwnd,
        (HMENU)ID_GENERATEBUTTON,
        appState->hInstance,
        0L);

    appState->hLayoutTree = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_TREEVIEW,
        "",
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE |
        TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
        10, 32 + 10 + 32 + 20 + 20,
        280, 170,
        hwnd,
        NULL,
        appState->hInstance,
        0L);

    TVINSERTSTRUCT tis = {0};
    tis.hInsertAfter = TVI_ROOT;
    tis.itemex.mask = TVIF_TEXT|TVIF_PARAM;
    tis.itemex.pszText = "root";
    tis.itemex.lParam = 0L;
    appState->hRootTreeItem = TreeView_InsertItem(appState->hLayoutTree, &tis);
    TreeView_SelectItem(appState->hLayoutTree, appState->hRootTreeItem);
    TreeView_Expand(appState->hLayoutTree, appState->hRootTreeItem, TVE_EXPAND);

    GroupBox_Create(appState->hInstance, hwnd,
            "Layout",
            0, 32+10+32+20,
            300, 200);

    HWND propertiesContainer = ScrollView_Create(appState->hInstance,
                                                 hwnd,
                                                 10, 32 + 10 + 32 + 10 + 200 + 20 + 20,
                                                 280, 170);

    CreateProperties(appState->hInstance, propertiesContainer);
    SendMessage(propertiesContainer, SVM_UPDATESCROLL, 0, 0);

    GroupBox_Create(appState->hInstance,
            hwnd,
            "Properties",
            0, 32+10+32+10+200+20,
            300, 200);

    appState->hLayoutView = LayoutView_Create(appState->hInstance,
                                              hwnd,
                                              310, 10,
                                              rc.right - rc.left - 200 - 20, rc.bottom - rc.top - 20);

    NONCLIENTMETRICS ncm = {sizeof(ncm)};
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

    appState->hFont = CreateFontIndirect(&ncm.lfMessageFont);
    ApplyFont(hwnd, appState->hFont);
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
                 SWP_NOMOVE | SWP_NOZORDER);
}

static void OnAdd(AppState* appState, HWND hwnd, HWND hButton)
{
    HTREEITEM parent = TreeView_GetSelection(appState->hLayoutTree);
    if (!parent)
    {
        MessageBox(hwnd, "No parent selected", "Error", MB_OK|MB_ICONERROR);
        return;
    }

    char label[32];
    snprintf(label, sizeof(label), "%d", ++appState->index);

    TVINSERTSTRUCT tis = {0};
    tis.hParent = parent;
    tis.hInsertAfter = TVI_LAST;
    tis.itemex.mask = TVIF_TEXT|TVIF_PARAM;
    tis.itemex.pszText = label;
    tis.itemex.lParam = (LPARAM)appState->index;
    HTREEITEM hNewItem = TreeView_InsertItem(appState->hLayoutTree, &tis);
    TreeView_Expand(appState->hLayoutTree, parent, TVE_EXPAND);
    TreeView_SelectItem(appState->hLayoutTree, hNewItem);
}

static void OnCommand(AppState* appState, HWND hwnd, HWND hButton, unsigned buttonID)
{
    switch (buttonID)
    {
    case ID_ADDBUTTON:
        OnAdd(appState, hwnd, hButton);
        break;
    case ID_GENERATEBUTTON:
        break;
    }
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    AppState* appState = (AppState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case WM_CREATE:
        OnCreate(hwnd, ((CREATESTRUCTA*)lparam)->lpCreateParams);
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_SIZE:
        if (appState)
        {
            OnSize(appState, hwnd, LOWORD(lparam), HIWORD(lparam));
        }
        return 0;
    case WM_COMMAND:
        if (appState)
        {
            OnCommand(appState, hwnd, (HWND)lparam, LOWORD(wparam));
        }
        return 0;
    case WM_DESTROY:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
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

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(wc);
    wc.hInstance = hInstance;
    wc.lpszClassName = APPNAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpfnWndProc = WindowProc;
    if (!RegisterClassEx(&wc))
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

    HWND hwnd = CreateWindow(
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
    appState.index = 0;

    if (!InitApplication(hInstance))
    {
        return 1;
    }
    else if (!InitInstance(hInstance, nShowCmd, &appState))
    {
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
