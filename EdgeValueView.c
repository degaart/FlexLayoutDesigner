#include "EdgeValueView.h"

#include "Trace.h"
#include "Util.h"

#define CLASSNAME "EdgeValueView"

#define EDGE_MIN YGEdgeLeft
#define EDGE_MAX (YGEdgeBottom+1)

enum ControlIDs
{
    ID_LEFT_LABEL = 101,
    ID_LEFT_EDIT,
};

typedef struct EdgeValueViewState
{
    unsigned signature;
    YGNodeRef flex;
    struct
    {
        HWND hLabel;
        HWND hControl;
        YGNodeRef flex;
        YGNodeRef labelFlex;
        YGNodeRef controlFlex;
    } controls[4];
    int height;
} EdgeValueViewState;

static void OnCreate(HWND hwnd)
{
    HMODULE hInstance = GetModuleHandle(NULL);

    EdgeValueViewState* state = calloc(1, sizeof(EdgeValueViewState));
    state->signature = 0xDEADBEEF;

    state->flex = CreateFlex(32.0f, NAN, NULL, NULL);
    YGNodeStyleSetFlexDirection(state->flex, YGFlexDirectionColumn);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)state);

    char flexLabel[64];
    for (int i = EDGE_MIN; i < EDGE_MAX; i++)
    {
        snprintf(flexLabel, sizeof(flexLabel), "controls[%d].flex", i);
        state->controls[i].flex = CreateFlex(NAN, 22.0f, flexLabel, NULL);
        YGNodeStyleSetFlexDirection(state->controls[i].flex, YGFlexDirectionRow);
        YGNodeInsertChild(state->flex, state->controls[i].flex, YGNodeGetChildCount(state->flex));

        const char* text;
        switch (i)
        {
        case YGEdgeLeft:
            text = "left";
            break;
        case YGEdgeTop:
            text = "top";
            break;
        case YGEdgeRight:
            text = "right";
            break;
        case YGEdgeBottom:
            text = "bottom";
            break;
        default:
            assert(!"Invalid code path");
            break;
        }
        state->controls[i].hLabel = CreateWindowEx(
            0,
            "STATIC",
            text,
            WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
            0, 0,
            45, 45,
            hwnd,
            (HMENU)ID_LEFT_LABEL,
            hInstance,
            0L);

        snprintf(flexLabel, sizeof(flexLabel), "controls[%d].labelFlex", i);
        state->controls[i].labelFlex = CreateFlex(45.0f, NAN, flexLabel, state->controls[i].hLabel);
        YGNodeInsertChild(state->controls[i].flex, state->controls[i].labelFlex, YGNodeGetChildCount(state->controls[i].flex));

        state->controls[i].hControl = CreateWindowEx(
            0,
            "EDIT",
            "",
            WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|WS_BORDER|ES_RIGHT,
            45, 0,
            45, 24,
            hwnd,
            (HMENU)ID_LEFT_EDIT,
            hInstance,
            0L);

        snprintf(flexLabel, sizeof(flexLabel), "controls[%d].controlFlex", i);
        state->controls[i].controlFlex = CreateFlex(NAN, 22.0f, flexLabel, state->controls[i].hControl);
        YGNodeStyleSetFlexGrow(state->controls[i].controlFlex, 1.0f);
        YGNodeInsertChild(state->controls[i].flex, state->controls[i].controlFlex, YGNodeGetChildCount(state->controls[i].flex));
    }

    YGNodeCalculateLayout(state->flex, YGUndefined, YGUndefined, YGDirectionLTR);
    LayoutFlex(state->flex, 0.0f, 0.0f);
    state->height = roundf(YGNodeLayoutGetHeight(state->flex));
}

static void OnSize(EdgeValueViewState* state, HWND hwnd, int w, int h)
{
    YGNodeStyleSetWidth(state->flex, w);
    YGNodeStyleSetHeight(state->flex, h);
    YGNodeCalculateLayout(state->flex, YGUndefined, YGUndefined, YGDirectionLTR);
    LayoutFlex(state->flex, 0.0f, 0.0f);
}

static void OnDestroy(EdgeValueViewState* state)
{
    DestroyFlex(state->flex);
    free(state);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    EdgeValueViewState* state;
    switch (msg)
    {
    case WM_CREATE:
        OnCreate(hwnd);
        return 0;
    case WM_SIZE:
        state = (EdgeValueViewState*)GetWindowLongPtr(hwnd, GWL_USERDATA);
        if (state)
        {
            OnSize(state, hwnd, LOWORD(lparam), HIWORD(lparam));
        }
        return 0;
    case WM_DESTROY:
        state = (EdgeValueViewState*)GetWindowLongPtr(hwnd, GWL_USERDATA);
        OnDestroy(state);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

bool EdgeValueView_Init(HINSTANCE hInstance)
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

HWND EdgeValueView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h)
{
    HWND hwnd = CreateWindowEx(
        0,
        CLASSNAME,
        "",
        WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_VISIBLE,
        x, y, 
        w, h,
        hParent,
        NULL,
        hInstance,
        0L);
    return hwnd;
}

void EdgeValueView_SetValue(HWND hControl, YGEdge edge, YGValue value)
{
    
}

int EdgeValueView_GetHeight(HWND hControl)
{
    const EdgeValueViewState* state = (const EdgeValueViewState*)GetWindowLongPtr(hControl, GWLP_USERDATA);
    assert(state != NULL);
    return state->height;
}
