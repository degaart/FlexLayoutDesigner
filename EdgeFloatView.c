#include "EdgeFloatView.h"

#include "Trace.h"
#include "Util.h"
#include "ValueView.h"

#define CLASSNAME "EdgeFloatView"

#define EDGE_MIN YGEdgeLeft
#define EDGE_MAX (YGEdgeBottom+1)

typedef struct EdgeFloatViewState
{
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
} EdgeFloatViewState;

static void OnCreate(HWND hwnd)
{
    HMODULE hInstance = GetModuleHandle(NULL);

    EdgeFloatViewState* state = calloc(1, sizeof(EdgeFloatViewState));

    state->flex = CreateFlex(32.0f, NAN, NULL, NULL);
    YGNodeStyleSetFlexDirection(state->flex, YGFlexDirectionColumn);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)state);

    char flexLabel[64];
    for (int i = EDGE_MIN; i < EDGE_MAX; i++)
    {
        snprintf(flexLabel, sizeof(flexLabel), "controls[%d].flex", i);
        state->controls[i].flex = CreateFlex(NAN, 22.0f, flexLabel, NULL);
        YGNodeStyleSetFlexDirection(state->controls[i].flex, YGFlexDirectionRow);
        YGNodeStyleSetMargin(state->controls[i].flex, YGEdgeTop, 2.0f);
        YGNodeInsertChild(state->flex, state->controls[i].flex, YGNodeGetChildCount(state->flex));

        const char* text;
        switch (i)
        {
        case YGEdgeLeft:
            text = "left:";
            break;
        case YGEdgeTop:
            text = "top:";
            break;
        case YGEdgeRight:
            text = "right:";
            break;
        case YGEdgeBottom:
            text = "bottom:";
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
            NULL,
            hInstance,
            0L);

        snprintf(flexLabel, sizeof(flexLabel), "controls[%d].labelFlex", i);
        state->controls[i].labelFlex = CreateFlex(45.0f, NAN, flexLabel, state->controls[i].hLabel);
        YGNodeInsertChild(state->controls[i].flex, state->controls[i].labelFlex, YGNodeGetChildCount(state->controls[i].flex));

        state->controls[i].hControl = CreateWindow(
            "EDIT",
            "",
            WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|ES_RIGHT,
            0, 0,
            32, 32,
            hwnd,
            NULL,
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

static void OnSize(EdgeFloatViewState* state, HWND hwnd, int w, int h)
{
    YGNodeStyleSetWidth(state->flex, w);
    YGNodeStyleSetHeight(state->flex, h);
    YGNodeCalculateLayout(state->flex, YGUndefined, YGUndefined, YGDirectionLTR);
    LayoutFlex(state->flex, 0.0f, 0.0f);
}

static void OnDestroy(EdgeFloatViewState* state)
{
    DestroyFlex(state->flex);
    free(state);
}

static void OnEditControlChange(EdgeFloatViewState* state, HWND hwnd, HWND hControl, WORD controlID)
{
    char buffer[128];
    GetWindowText(hControl, buffer, sizeof(buffer));

    float value;
    if (!ParseFloat(buffer, &value))
    {
        return;
    }

    for (int i = EDGE_MIN; i < EDGE_MAX; i++)
    {
        if (state->controls[i].hControl == hControl)
        {
            NMEDGEFLOATVIEW nmevv = {0};
            nmevv.hdr.code = EVVN_CHANGED;
            nmevv.hdr.hwndFrom = hwnd;
            nmevv.hdr.idFrom = GetWindowLongPtr(hwnd, GWLP_ID);
            nmevv.edge = i;
            nmevv.value = value;
            SendMessage(GetParent(hwnd), WM_NOTIFY, nmevv.hdr.idFrom, (LPARAM)&nmevv);
        }
    }
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    EdgeFloatViewState* state;
    switch (msg)
    {
    case WM_CREATE:
        OnCreate(hwnd);
        return 0;
    case WM_SIZE:
        state = (EdgeFloatViewState*)GetWindowLongPtr(hwnd, GWL_USERDATA);
        if (state)
        {
            OnSize(state, hwnd, LOWORD(lparam), HIWORD(lparam));
        }
        return 0;
    case WM_DESTROY:
        state = (EdgeFloatViewState*)GetWindowLongPtr(hwnd, GWL_USERDATA);
        OnDestroy(state);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        return 0;
    case WM_COMMAND:
        state = (EdgeFloatViewState*)GetWindowLongPtr(hwnd, GWL_USERDATA);
        if (state)
        {
            switch (HIWORD(wparam))
            {
            case EN_CHANGE:
                OnEditControlChange(state, hwnd, (HWND)lparam, LOWORD(wparam));
                break;
            }
        }
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

bool EdgeFloatView_Init(HINSTANCE hInstance)
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

HWND EdgeFloatView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h)
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

void EdgeFloatView_SetValue(HWND hControl, YGEdge edge, float value)
{
    assert(edge >= EDGE_MIN && edge <= EDGE_MAX);

    EdgeFloatViewState* state = (EdgeFloatViewState*)GetWindowLongPtr(hControl, GWLP_USERDATA);
    assert(state != NULL);

    char text[128];
    snprintf(text, sizeof(text), "%0.f", value);
    SetWindowText(state->controls[edge].hControl, text);
}

int EdgeFloatView_GetHeight(HWND hControl)
{
    const EdgeFloatViewState* state = (const EdgeFloatViewState*)GetWindowLongPtr(hControl, GWLP_USERDATA);
    assert(state != NULL);
    return state->height;
}
